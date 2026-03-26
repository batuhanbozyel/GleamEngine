#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/Renderer.h"
#include "Renderer/Swapchain.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"
#include "Renderer/RayTracingScene.h"

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RenderGraphTextureNode* node, RenderGraphPassNode* pass)
{
    if (node->creator == pass || !node->transient)
    {
        return node->clearBuffer ? AttachmentLoadAction::Clear : AttachmentLoadAction::DontCare;
    }
    return AttachmentLoadAction::Load;
}

static AttachmentStoreAction GetStoreActionForRenderTexture(const RenderGraphTextureNode* node, RenderGraphPassNode* pass)
{
    return (node->lastReference == pass && !pass->hasSideEffect) ? AttachmentStoreAction::DontCare : AttachmentStoreAction::Store;
}

RenderGraph::RenderGraph(const RenderGraphContext& context)
    : mContext(context)
{
    
}

RenderGraph::~RenderGraph()
{
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

void RenderGraph::Compile()
{
    // Setup resource dependency
    for (auto pass : mPassNodes)
    {
		// Buffer
        for (auto& resource : pass->bufferReads)
        {
            for (auto producer : resource.node->producers)
            {
                auto [_, success] = producer->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }
        
        for (auto& resource : pass->bufferWrites)
        {
            if (resource.node->creator && resource.node->creator != pass)
            {
                auto [_, success] = resource.node->creator->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }

		// Texture
		for (auto& resource : pass->textureReads)
		{
			for (auto producer : resource.node->producers)
			{
				auto [_, success] = producer->dependents.insert(pass);
				pass->refCount += static_cast<uint32_t>(success);
			}
		}
        
        for (auto& resource : pass->textureWrites)
        {
            if (resource.node->creator && resource.node->creator != pass)
            {
                auto [_, success] = resource.node->creator->dependents.insert(pass);
                pass->refCount += static_cast<uint32_t>(success);
            }
        }
    }
    
    // Perform topological sort
	auto cmp = [](RenderGraphPassNode* a, RenderGraphPassNode* b)
	{
		return a->uniqueId > b->uniqueId;
	};
    PriorityQueue<RenderGraphPassNode*, TArray<RenderGraphPassNode*>, decltype(cmp)> passQueue(cmp);
    TArray<RenderGraphPassNode*> sortedPasses;
    for (auto pass : mPassNodes)
    {
        if (pass->refCount == 0)
        {
            passQueue.push(pass);
        }
    }
    
    while (!passQueue.empty())
    {
        auto pass = passQueue.top();
        passQueue.pop();
        
        sortedPasses.push_back(pass);
        for (auto dependent : pass->dependents)
        {
            if (--dependent->refCount == 0)
            {
                passQueue.push(dependent);
            }
        }
    }
    mPassNodes = sortedPasses;
    
    // Calculate resource lifetimes
    for (auto pass : mPassNodes)
    {
		// Buffer
        for (auto& resource : pass->bufferWrites)
        {
            resource.node->lastModifier = pass;
            resource.node->lastReference = pass;
        }
        for (auto& resource : pass->bufferReads)
        {
			resource.node->lastReference = pass;
        }

		// Texture
		for (auto& resource : pass->textureWrites)
		{
			resource.node->lastModifier = pass;
			resource.node->lastReference = pass;
		}
		for (auto& resource : pass->textureReads)
		{
			resource.node->lastReference = pass;
		}
    }
}

void RenderGraph::Execute(const CommandBuffer* cmd, SceneRenderingData& sceneData)
{
	RayTracingScene rtScene(mContext.device, mContext.allocator);
	rtScene.BuildAccelerationStructure(cmd, sceneData.sceneProxy);

    for (auto pass : mPassNodes)
    {
		AllocatePassResources(pass, cmd, sceneData);
		SetupPassBarriers(pass, cmd);
		ExecutePass(pass, cmd);
		FreePassResources(pass, cmd);
    }
}

void RenderGraph::AllocatePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd, SceneRenderingData& sceneData)
{
	// Allocate buffers
	for (uint32_t i = 0; i < pass->bufferCreates.size(); i++)
	{
		auto& resource = pass->bufferCreates[i];
		if (HasResource(pass->bufferWrites, resource))
		{
			TStringStream name;
			auto descriptor = resource.node->buffer.GetDescriptor();
			descriptor.name.empty() ? (name << pass->name << "::Buffer[" << i << "]")
				: (name << pass->name << "::" << descriptor.name);
			descriptor.name = name.str();

			resource.node->buffer = mContext.device->CreateBuffer(mContext.allocator, descriptor);
			GLEAM_ASSERT(resource.node->buffer.IsValid());
		}
	}

	// Allocate textures
	for (uint32_t i = 0; i < pass->textureCreates.size(); i++)
	{
		auto& resource = pass->textureCreates[i];
		if (HasResource(pass->textureWrites, resource))
		{
			TStringStream name;
			auto descriptor = resource.node->texture.GetDescriptor();
			descriptor.name.empty() ? (name << pass->name << "::Texture[" << i << "]")
				: (name << pass->name << "::" << descriptor.name);
			descriptor.name = name.str();

			resource.node->texture = mContext.device->CreateTexture(mContext.allocator, descriptor);
			resource.node->barrierState.layout = BarrierLayout::Common;
			GLEAM_ASSERT(resource.node->texture.IsValid());
		}
	}

	// Acquire backbuffer texture
	for (uint32_t i = 0; i < pass->textureWrites.size(); i++)
	{
		auto& resource = pass->textureWrites[i];
		if (resource == sceneData.backbuffer)
		{
			sceneData.backbuffer.node->texture = static_cast<Swapchain*>(mContext.surface)->AcquireNextDrawable();
			resource.node->texture = sceneData.backbuffer.node->texture;
			resource.node->barrierState.layout = BarrierLayout::Common;
		}
	}
}

void RenderGraph::FreePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd)
{
	for (auto& resource : pass->bufferReads)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			mContext.device->Dispose(mContext.allocator, resource.node->buffer);
		}
	}

	for (auto& resource : pass->textureReads)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			mContext.device->Dispose(mContext.allocator, resource.node->texture);
		}
	}

	for (auto& resource : pass->bufferWrites)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			mContext.device->Dispose(mContext.allocator, resource.node->buffer);
		}
	}

	for (auto& resource : pass->textureWrites)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			mContext.device->Dispose(mContext.allocator, resource.node->texture);
		}
	}
}

void RenderGraph::SetupPassBarriers(RenderGraphPassNode* pass, const CommandBuffer* cmd)
{
	BarrierGroup barrier;
	barrier.bufferBarriers.reserve(pass->bufferReads.size() + pass->bufferWrites.size());
	for (auto& resource : pass->bufferReads)
	{
		if (resource.node->barrierState.access == BarrierAccess::ShaderResource)
		{
			continue;
		}

		BufferBarrier bufferBarrier;
		bufferBarrier.resource = resource.node->buffer.GetHandle();
		bufferBarrier.srcStage = resource.node->barrierState.stage;
		bufferBarrier.dstStage = BarrierStage::AllShading;
		bufferBarrier.srcAccess = resource.node->barrierState.access;
		bufferBarrier.dstAccess = BarrierAccess::ShaderResource;
		barrier.bufferBarriers.push_back(bufferBarrier);

		resource.node->barrierState.stage = bufferBarrier.dstStage;
		resource.node->barrierState.access = bufferBarrier.dstAccess;
	}

	for (auto& resource : pass->bufferWrites)
	{
		BufferBarrier bufferBarrier;
		bufferBarrier.resource = resource.node->buffer.GetHandle();
		bufferBarrier.srcStage = resource.node->barrierState.stage;
		bufferBarrier.dstStage = BarrierStage::AllShading;
		bufferBarrier.srcAccess = resource.node->barrierState.access;
		bufferBarrier.dstAccess = BarrierAccess::UnorderedAccess;
		barrier.bufferBarriers.push_back(bufferBarrier);

		resource.node->barrierState.stage = bufferBarrier.dstStage;
		resource.node->barrierState.access = bufferBarrier.dstAccess;
	}

	barrier.textureBarriers.reserve(pass->textureReads.size() + pass->textureWrites.size());
	for (auto& resource : pass->textureReads)
	{
		BarrierStage dstStage = BarrierStage::AllShading;
		BarrierAccess dstAccess = BarrierAccess::ShaderResource;
		BarrierLayout newLayout = BarrierLayout::ShaderResource;

		const auto& textureDesc = resource.node->texture.GetDescriptor();
		if (pass->type == RenderGraphPassType::Raster)
		{
			if (Utils::IsDepthFormat(textureDesc.format))
			{
				dstStage = BarrierStage::DepthStencil;
				dstAccess = BarrierAccess::DepthStencilRead;
				newLayout = BarrierLayout::DepthStencilRead;
			}
		}
		else
		{
			dstStage = BarrierStage::ComputeShading;
		}

		if (resource.node->barrierState.access == dstAccess)
		{
			continue;
		}

		TextureBarrier textureBarrier;
		textureBarrier.resource = resource.node->texture.GetHandle();
		textureBarrier.srcStage = resource.node->barrierState.stage;
		textureBarrier.dstStage = dstStage;
		textureBarrier.srcAccess = resource.node->barrierState.access;
		textureBarrier.dstAccess = dstAccess;
		textureBarrier.oldLayout = resource.node->barrierState.layout;
		textureBarrier.newLayout = newLayout;
		barrier.textureBarriers.push_back(textureBarrier);

		resource.node->barrierState.stage = textureBarrier.dstStage;
		resource.node->barrierState.access = textureBarrier.dstAccess;
		resource.node->barrierState.layout = textureBarrier.newLayout;
	}

	for (auto& resource : pass->textureWrites)
	{
		BarrierStage dstStage = BarrierStage::AllShading;
		BarrierAccess dstAccess = BarrierAccess::UnorderedAccess;
		BarrierLayout newLayout = BarrierLayout::UnorderedAccess;

		const auto& textureDesc = resource.node->texture.GetDescriptor();
		if (pass->type == RenderGraphPassType::Raster)
		{
			if (pass->depthAttachment.node == resource.node)
			{
				dstStage = BarrierStage::DepthStencil;
				dstAccess = BarrierAccess::DepthStencilWrite;
				newLayout = BarrierLayout::DepthStencilWrite;
			}
			else
			{
				dstStage = BarrierStage::RenderTarget;
				dstAccess = BarrierAccess::RenderTarget;
				newLayout = BarrierLayout::RenderTarget;
			}
		}
		else if (pass->type == RenderGraphPassType::Compute)
		{
			dstStage = BarrierStage::ComputeShading;
		}

		TextureBarrier textureBarrier;
		textureBarrier.resource = resource.node->texture.GetHandle();
		textureBarrier.srcStage = resource.node->barrierState.stage;
		textureBarrier.dstStage = dstStage;
		textureBarrier.srcAccess = resource.node->barrierState.access;
		textureBarrier.dstAccess = dstAccess;
		textureBarrier.oldLayout = resource.node->barrierState.layout;
		textureBarrier.newLayout = newLayout;
		barrier.textureBarriers.push_back(textureBarrier);

		resource.node->barrierState.stage = textureBarrier.dstStage;
		resource.node->barrierState.access = textureBarrier.dstAccess;
		resource.node->barrierState.layout = textureBarrier.newLayout;
	}
	cmd->Barrier(barrier);
}

void RenderGraph::ExecutePass(RenderGraphPassNode* pass, const CommandBuffer* cmd)
{
	if (pass->type == RenderGraphPassType::Native)
	{
		std::invoke(pass->callback, cmd);
	}
	else if (pass->type == RenderGraphPassType::Raster)
	{
		if (pass->colorAttachments.empty() && pass->depthAttachment.IsValid() == false)
		{
			std::invoke(pass->callback, cmd);
		}
		else
		{
			RenderPassDescriptor renderPassDesc{};
			renderPassDesc.colorAttachments.resize(pass->colorAttachments.size());
			for (uint32_t i = 0; i < pass->colorAttachments.size(); i++)
			{
				const auto node = static_cast<const RenderGraphTextureNode*>(pass->colorAttachments[i].node);
				renderPassDesc.colorAttachments[i].texture = node->texture;
				renderPassDesc.colorAttachments[i].loadAction = GetLoadActionForRenderTexture(node, pass);
				renderPassDesc.colorAttachments[i].storeAction = GetStoreActionForRenderTexture(node, pass);
				renderPassDesc.colorAttachments[i].clearColor = node->clearColor;

				const auto& descriptor = renderPassDesc.colorAttachments[i].texture.GetDescriptor();
				renderPassDesc.size = descriptor.size;
			}

			if (pass->depthAttachment.IsValid())
			{
				const auto node = static_cast<const RenderGraphTextureNode*>(pass->depthAttachment.node);
				renderPassDesc.depthAttachment.texture = node->texture;
				renderPassDesc.depthAttachment.loadAction = GetLoadActionForRenderTexture(node, pass);
				renderPassDesc.depthAttachment.storeAction = pass->depthAccess == DepthAccess::Read ? AttachmentStoreAction::DontCare : GetStoreActionForRenderTexture(node, pass);
				renderPassDesc.depthAttachment.clearDepth = node->clearDepth;
				renderPassDesc.depthAttachment.clearStencil = node->clearStencil;

				const auto& descriptor = renderPassDesc.depthAttachment.texture.GetDescriptor();
				renderPassDesc.size = descriptor.size;
			}

			cmd->BeginRenderPass(renderPassDesc, pass->name);
			cmd->SetViewport(renderPassDesc.size);
			cmd->SetScissorRect(renderPassDesc.size);
			std::invoke(pass->callback, cmd);
			cmd->EndRenderPass();
		}
	}
	else if (pass->type == RenderGraphPassType::Compute)
	{
		cmd->BeginComputePass(pass->name);
		std::invoke(pass->callback, cmd);
		cmd->EndComputePass();
	}
}

TextureHandle RenderGraph::ImportTexture(const Texture& backbuffer, const ImportResourceParams& params)
{
	RenderTextureDescriptor descriptor(backbuffer.GetDescriptor());
	descriptor.clearBuffer = params.clearOnFirstUse;
	descriptor.clearColor = params.clearColor;

	auto handle = mRegistry.CreateTexture(descriptor, false);
    handle.node->texture = backbuffer;
    return handle;
}

const TextureDescriptor& RenderGraph::GetDescriptor(TextureHandle handle) const
{
	auto node = static_cast<const RenderGraphTextureNode*>(handle.node);
	return node->texture.GetDescriptor();
}

GPUAllocator* RenderGraph::GetAllocator() const
{
	return mContext.allocator;
}
