#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/Renderer.h"
#include "Renderer/Swapchain.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/GraphicsDevice.h"

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RenderGraphTextureNode* node, RenderGraphPassNode* pass)
{
    bool isFirstUse = node->creator == pass ||
                      (not node->transient && (node->producers.empty() || node->producers.front() == pass));
    if (isFirstUse)
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
	for (auto pass : mPassNodes)
	{
		// Buffer reads
		for (const auto& resource : pass->bufferReads)
		{
			const auto& producers = resource.node->producers;

			// Read-after-Write
			if (resource.version > 0)
			{
				auto producer = producers[resource.version - 1];
				if (producer != pass)
				{
					auto [_, inserted] = producer->dependents.insert(pass);
					pass->refCount += static_cast<uint32_t>(inserted);
				}
			}

			// Write-after-Read
			if (resource.version < producers.size())
			{
				auto nextWriter = producers[resource.version];
				if (nextWriter != pass)
				{
					auto [_, inserted] = pass->dependents.insert(nextWriter);
					nextWriter->refCount += static_cast<uint32_t>(inserted);
				}
			}
		}

		// Texture reads
		for (const auto& resource : pass->textureReads)
		{
			const auto& producers = resource.node->producers;

			// Read-after-Write
			if (resource.version > 0)
			{
				auto producer = producers[resource.version - 1];
				if (producer != pass)
				{
					auto [_, inserted] = producer->dependents.insert(pass);
					pass->refCount += static_cast<uint32_t>(inserted);
				}
			}

			// Write-after-Read
			if (resource.version < producers.size())
			{
				auto nextWriter = producers[resource.version];
				if (nextWriter != pass)
				{
					auto [_, inserted] = pass->dependents.insert(nextWriter);
					nextWriter->refCount += static_cast<uint32_t>(inserted);
				}
			}
		}

		// Buffer writes — Write-after-Write
		for (const auto& resource : pass->bufferWrites)
		{
			const auto& producers = resource.node->producers;
			if (resource.version > 1)
			{
				auto prevWriter = producers[resource.version - 2];
				if (prevWriter != pass)
				{
					auto [_, inserted] = prevWriter->dependents.insert(pass);
					pass->refCount += static_cast<uint32_t>(inserted);
				}
			}
		}

		// Texture writes — Write-after-Write
		for (const auto& resource : pass->textureWrites)
		{
			const auto& producers = resource.node->producers;
			if (resource.version > 1)
			{
				auto prevWriter = producers[resource.version - 2];
				if (prevWriter != pass)
				{
					auto [_, inserted] = prevWriter->dependents.insert(pass);
					pass->refCount += static_cast<uint32_t>(inserted);
				}
			}
		}
	}
	
	auto cmp = [](RenderGraphPassNode* lhs, RenderGraphPassNode* rhs)
	{
		return lhs->uniqueId > rhs->uniqueId;
	};
	PriorityQueue<RenderGraphPassNode*, TArray<RenderGraphPassNode*>, decltype(cmp)> readyQueue(cmp);
	for (auto pass : mPassNodes)
	{
		if (pass->refCount == 0)
		{
			readyQueue.push(pass);
		}
	}

	TArray<RenderGraphPassNode*> sortedPasses;
	sortedPasses.reserve(mPassNodes.size());
	while (not readyQueue.empty())
	{
		auto pass = readyQueue.top();
		readyQueue.pop();

		sortedPasses.push_back(pass);
		for (auto dependent : pass->dependents)
		{
			if (--dependent->refCount == 0)
			{
				readyQueue.push(dependent);
			}
		}
	}
	GLEAM_ASSERT(sortedPasses.size() == mPassNodes.size(), "RenderGraph::Compile: cyclic pass dependency detected");
	mPassNodes = sortedPasses;

	// Calculate resource lifetimes in execution order
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

			const auto& alloc = mContext.allocator->GetAllocation(resource.node->buffer.GetHandle());
			resource.node->barrierState.stage = alloc.aliasStage;
			resource.node->barrierState.access = BarrierAccess::None;
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
			GLEAM_ASSERT(resource.node->texture.IsValid());

			const auto& alloc = mContext.allocator->GetAllocation(resource.node->texture.GetHandle());
			resource.node->barrierState.stage = alloc.aliasStage;
			resource.node->barrierState.access = BarrierAccess::None;
			resource.node->barrierState.layout = BarrierLayout::Undefined;
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
			resource.node->barrierState = {};
		}
	}
}

void RenderGraph::FreePassResources(RenderGraphPassNode* pass, const CommandBuffer* cmd)
{
	BarrierGroup barrier;
	for (auto& resource : pass->bufferReads)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			barrier.bufferBarriers.push_back({
				.resource = resource.node->buffer.GetHandle(),
				.srcStage = resource.node->barrierState.stage,
				.dstStage = BarrierStage::None,
				.srcAccess = resource.node->barrierState.access,
				.dstAccess = BarrierAccess::None
			});
			mContext.device->Dispose(mContext.allocator, resource.node->buffer, resource.node->barrierState.stage);
		}
	}

	for (auto& resource : pass->textureReads)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			barrier.textureBarriers.push_back({
				.resource = resource.node->texture.GetHandle(),
				.srcStage = resource.node->barrierState.stage,
				.dstStage = BarrierStage::None,
				.srcAccess = resource.node->barrierState.access,
				.dstAccess = BarrierAccess::None,
				.oldLayout = resource.node->barrierState.layout,
				.newLayout = BarrierLayout::Undefined
			});
			mContext.device->Dispose(mContext.allocator, resource.node->texture, resource.node->barrierState.stage);
		}
	}

	for (auto& resource : pass->bufferWrites)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			barrier.bufferBarriers.push_back({
				.resource = resource.node->buffer.GetHandle(),
				.srcStage = resource.node->barrierState.stage,
				.dstStage = BarrierStage::None,
				.srcAccess = resource.node->barrierState.access,
				.dstAccess = BarrierAccess::None
			});
			mContext.device->Dispose(mContext.allocator, resource.node->buffer, resource.node->barrierState.stage);
		}
	}

	for (auto& resource : pass->textureWrites)
	{
		if (resource.node->lastReference == pass && resource.node->transient)
		{
			barrier.textureBarriers.push_back({
				.resource = resource.node->texture.GetHandle(),
				.srcStage = resource.node->barrierState.stage,
				.dstStage = BarrierStage::None,
				.srcAccess = resource.node->barrierState.access,
				.dstAccess = BarrierAccess::None,
				.oldLayout = resource.node->barrierState.layout,
				.newLayout = BarrierLayout::Undefined
			});
			mContext.device->Dispose(mContext.allocator, resource.node->texture, resource.node->barrierState.stage);
		}
	}
	cmd->Barrier(barrier);
}

void RenderGraph::SetupPassBarriers(RenderGraphPassNode* pass, const CommandBuffer* cmd)
{
	BarrierGroup barrier;
	barrier.bufferBarriers.reserve(pass->bufferReads.size() + pass->bufferWrites.size());
	for (auto& resource : pass->bufferReads)
	{
		BarrierStage dstStage = BarrierStage::AllShading;
		BarrierAccess dstAccess= BarrierAccess::ShaderResource;

		if (resource.node->buffer.GetDescriptor().usage == BufferUsage::IndirectArgument)
		{
			dstStage = BarrierStage::ExecuteIndirect;
			dstAccess = BarrierAccess::IndirectArgument;
		}

		if (resource.node->barrierState.access == dstAccess)
		{
			continue;
		}

		BufferBarrier bufferBarrier;
		bufferBarrier.resource = resource.node->buffer.GetHandle();
		bufferBarrier.srcStage = resource.node->barrierState.stage;
		bufferBarrier.dstStage = dstStage;
		bufferBarrier.srcAccess = resource.node->barrierState.access;
		bufferBarrier.dstAccess = dstAccess;
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
		textureBarrier.discard = resource.node->creator == pass;
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
