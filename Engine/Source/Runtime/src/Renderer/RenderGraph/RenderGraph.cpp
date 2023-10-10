#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RendererContext.h"

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RenderGraphTextureNode* node, RenderPassNode* pass)
{
    if (node->creator == pass || !node->transient)
    {
        return node->clearBuffer ? AttachmentLoadAction::Clear : AttachmentLoadAction::DontCare;
    }
    return AttachmentLoadAction::Load;
}

static AttachmentStoreAction GetStoreActionForRenderTexture(const RenderGraphTextureNode* node, RenderPassNode* pass)
{
    if (node->texture.GetDescriptor().sampleCount > 1)
    {
        return node->lastModifier == pass ? AttachmentStoreAction::Resolve : AttachmentStoreAction::StoreAndResolve;
    }
    return (node->lastReference == pass && !pass->hasSideEffect) ? AttachmentStoreAction::DontCare : AttachmentStoreAction::Store;
}

RenderGraph::RenderGraph(RendererContext& context)
    : mContext(context)
{
    
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

		// Texture
		for (auto& resource : pass->textureReads)
		{
			for (auto producer : resource.node->producers)
			{
				auto [_, success] = producer->dependents.insert(pass);
				pass->refCount += static_cast<uint32_t>(success);
			}
		}
    }
    
    // Perform topological sort
    Queue<RenderPassNode*> passQueue;
    TArray<RenderPassNode*> sortedPasses;
    for (auto pass : mPassNodes)
    {
        if (pass->refCount == 0) { passQueue.push(pass); }
    }
    
    while (!passQueue.empty())
    {
        auto pass = passQueue.front();
        passQueue.pop();
        
        sortedPasses.push_back(pass);
        for (auto dependent : pass->dependents)
        {
            if (--dependent->refCount == 0) { passQueue.push(dependent); }
        }
    }
    mPassNodes = sortedPasses;
    
    // Calculate resource lifetimes
    for (auto pass : mPassNodes)
    {
		// Buffer
        for (auto& resource : pass->bufferCreates)
        {
            auto node = static_cast<RenderGraphBufferNode*>(resource.node);
            mHeapSize += node->buffer.GetDescriptor().size;
            
            resource.node->creator = pass;
            resource.node->lastModifier = pass;
            resource.node->lastReference = pass;
        }
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
		for (auto& resource : pass->textureCreates)
		{
			resource.node->creator = pass;
			resource.node->lastModifier = pass;
			resource.node->lastReference = pass;
		}
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

void RenderGraph::Execute(const CommandBuffer* cmd)
{
    Heap heap;
    if (mHeapSize > 0)
    {
        heap = mContext.CreateHeap({ .memoryType = MemoryType::GPU, .size = mHeapSize });
    }
    size_t bufferOffset = 0;
    
    cmd->Begin();
    for (auto pass : mPassNodes)
    {
		// Allocate buffers
        for (auto& resource : pass->bufferCreates)
        {
            if (HasResource(pass->bufferWrites, resource))
            {
                resource.node->buffer = heap.CreateBuffer(resource.node->buffer.GetDescriptor(), bufferOffset);
                bufferOffset += resource.node->buffer.GetDescriptor().size;
            }
        }

		// Allocate textures
		for (auto& resource : pass->textureCreates)
		{
            if (HasResource(pass->textureWrites, resource))
            {
                resource.node->texture = mContext.CreateTexture(resource.node->texture.GetDescriptor());
            }
		}

		for (auto& resource : pass->textureReads)
		{
			cmd->TransitionLayout(resource.node->texture, ResourceAccess::Read);
		}
        
        // execute render pass
        if (pass->isCustomPass())
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
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            if (pass->depthAttachment.IsValid())
            {
				const auto node = static_cast<const RenderGraphTextureNode*>(pass->depthAttachment.node);
                renderPassDesc.depthAttachment.texture = node->texture;
                renderPassDesc.depthAttachment.loadAction = GetLoadActionForRenderTexture(node, pass);
                renderPassDesc.depthAttachment.storeAction = GetStoreActionForRenderTexture(node, pass);
                renderPassDesc.depthAttachment.clearDepth = node->clearDepth;
                renderPassDesc.depthAttachment.clearStencil = node->clearStencil;
                
                const auto& descriptor = renderPassDesc.depthAttachment.texture.GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            cmd->BeginRenderPass(renderPassDesc, pass->name);
			cmd->SetViewport(renderPassDesc.size);
            std::invoke(pass->callback, cmd);
            cmd->EndRenderPass();
        }
        
        // Release textures
        for (auto& resource : pass->textureWrites)
        {
            if (resource.node->lastReference == pass && resource.node->transient)
            {
                mContext.ReleaseTexture(resource.node->texture);
            }
        }
        
        for (auto& resource : pass->textureReads)
        {
            if (resource.node->lastReference == pass && resource.node->transient)
            {
                mContext.ReleaseTexture(resource.node->texture);
            }
        }
    }
    cmd->End();
    
    // Release buffers
    for (auto pass : mPassNodes)
    {
        for (auto& resource : pass->bufferCreates)
        {
			resource.node->buffer.Dispose(); // TODO: wait for frame to finish rendering
        }
    }
    
    if (heap.IsValid())
    {
        mContext.ReleaseHeap(heap);
    }
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

TextureHandle RenderGraph::ImportBackbuffer(const Texture& backbuffer, const ImportResourceParams& params)
{
	RenderTextureDescriptor descriptor(backbuffer.GetDescriptor());
	descriptor.clearBuffer = params.clearOnFirstUse;
	descriptor.clearColor = params.clearColor;

	auto handle = mRegistry.CreateTexture(descriptor, false);
    handle.node->texture = backbuffer;
    return handle;
}

const BufferDescriptor& RenderGraph::GetDescriptor(BufferHandle handle) const
{
	auto node = static_cast<const RenderGraphBufferNode*>(handle.node);
	return node->buffer.GetDescriptor();
}

const TextureDescriptor& RenderGraph::GetDescriptor(TextureHandle handle) const
{
	auto node = static_cast<const RenderGraphTextureNode*>(handle.node);
	return node->texture.GetDescriptor();
}
