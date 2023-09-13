#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"

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
    cmd->Begin();
    for (auto pass : mPassNodes)
    {
		// Allocate buffers
        for (auto& resource : pass->bufferCreates)
        {
			auto node = static_cast<RenderGraphBufferNode*>(resource.node);
			node->buffer = mStackAllocator.CreateBuffer(node->buffer.GetDescriptor());
        }

		// Allocate textures
		for (auto& resource : pass->textureCreates)
		{
			auto node = static_cast<RenderGraphTextureNode*>(resource.node);
			node->texture = mPoolAllocator.CreateTexture(node->texture.GetDescriptor());
		}
        
        // execute render pass
        if (pass->isCustomPass())
        {
            std::invoke(pass->callback, cmd);
        }
        else
        {
            RenderPassDescriptor renderPassDesc;
            renderPassDesc.colorAttachments.resize(pass->colorAttachments.size());
            for (uint32_t i = 0; i < pass->colorAttachments.size(); i++)
            {
                auto& attachment = pass->colorAttachments[i];
				auto node = static_cast<const RenderGraphTextureNode*>(attachment.node);
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
				auto node = static_cast<const RenderGraphTextureNode*>(pass->depthAttachment.node);
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
        
        // TODO: release resources that are not referenced in later passes
    }
    cmd->End();
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

TextureHandle RenderGraph::ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer, const ImportResourceParams& params)
{
	RenderTextureDescriptor descriptor;
    descriptor.size = backbuffer->GetDescriptor().size;
    descriptor.format = backbuffer->GetDescriptor().format;
    descriptor.usage = backbuffer->GetDescriptor().usage;
    descriptor.sampleCount = backbuffer->GetDescriptor().sampleCount;
    descriptor.useMipMap = backbuffer->GetDescriptor().useMipMap;
	descriptor.clearBuffer = params.clearOnFirstUse;
	descriptor.clearColor = params.clearColor;

	auto handle = mRegistry.CreateTexture(descriptor);
	auto node = static_cast<RenderGraphTextureNode*>(handle.node);
	node->texture = *backbuffer;
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
