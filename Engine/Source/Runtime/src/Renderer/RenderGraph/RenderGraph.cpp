#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RefCounted<RenderTexture>& renderTexture, const RenderGraphResourceNode& resource, RenderPassNode* pass)
{
    if (resource.creator == pass || !resource.transient)
    {
        return resource.descriptor.clearBuffer ? AttachmentLoadAction::Clear : AttachmentLoadAction::DontCare;
    }
    return AttachmentLoadAction::Load;
}

static AttachmentStoreAction GetStoreActionForRenderTexture(const RefCounted<RenderTexture>& renderTexture, const RenderGraphResourceNode& resource, RenderPassNode* pass)
{
    if (resource.GetDescriptor().sampleCount > 1)
    {
        return resource.lastModifier == pass ? AttachmentStoreAction::Resolve : AttachmentStoreAction::StoreAndResolve;
    }
    return (resource.lastReference == pass && !pass->hasSideEffect) ? AttachmentStoreAction::DontCare : AttachmentStoreAction::Store;
}

void RenderGraph::Compile()
{
    // Setup resource dependency
    for (auto pass : mPassNodes)
    {
        for (auto resource : pass->resourceReads)
        {
            const auto& consumed = mRegistry.GetResourceNode(resource);
            for (auto producer : consumed.producers)
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
        for (auto resource : pass->resourceCreates)
        {
            auto& node = mRegistry.GetResourceNode(resource);
            node.creator = pass;
            node.lastModifier = pass;
            node.lastReference = pass;
        }
        for (auto resource : pass->resourceWrites)
        {
            auto& node = mRegistry.GetResourceNode(resource);
            node.lastModifier = pass;
            node.lastReference = pass;
        }
        for (auto resource : pass->resourceReads)
        {
            mRegistry.GetResourceNode(resource).lastReference = pass;
        }
    }
}

void RenderGraph::Execute(const CommandBuffer* cmd)
{
    cmd->Begin();
    for (auto pass : mPassNodes)
    {
        size_t allocationSize = 0;
        for (auto resource : pass->resourceCreates)
        {
            auto& node = mRegistry.GetResourceNode(resource);
            if (node.type == RenderGraphResourceNode::Type::Buffer)
            {
                const auto& descriptor = std::get<BufferDescriptor>(node.descriptor);
                allocationSize += mStackAllocator.Allocate(descriptor);
            }
            // if RenderTexture, use RenderGraphPoolAllocator to allocate
            // else, use RenderGraphStackAllocator
        }
        
        // execute render pass
        if (pass->isCustomPass())
        {
            std::invoke(pass->callback, context);
        }
        else
        {
            RenderPassDescriptor renderPassDesc;
            renderPassDesc.colorAttachments.resize(pass->colorAttachments.size());
            for (uint32_t i = 0; i < pass->colorAttachments.size(); i++)
            {
                auto attachment = pass->colorAttachments[i];
                const auto& renderTexture = mRegistry.GetRenderTexture(attachment); // Retrieve from Pool
                renderPassDesc.colorAttachments[i].texture = renderTexture;
                
                const auto& node = mRegistry.GetResourceNode(attachment);
                renderPassDesc.colorAttachments[i].loadAction = GetLoadActionForRenderTexture(renderTexture, node, pass); // maybe store metadata next to RenderGraphResource
                renderPassDesc.colorAttachments[i].storeAction = GetStoreActionForRenderTexture(renderTexture, node, pass); // maybe store metadata next to RenderGraphResource
                renderPassDesc.colorAttachments[i].clearColor = node->descriptor.clearColor; // maybe store metadata next to RenderGraphResource
                
                const auto& descriptor = renderPassDesc.colorAttachments[i].texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            if (pass->depthAttachment != RenderGraphResource::nullHandle)
            {
                const auto& renderTexture = mRegistry.GetRenderTexture(pass->depthAttachment); // Retrieve from Pool
                renderPassDesc.depthAttachment.texture = renderTexture;
                
                const auto& node = mRegistry.GetResourceNode(pass->depthAttachment);
                renderPassDesc.depthAttachment.loadAction = GetLoadActionForRenderTexture(renderTexture, node, pass); // maybe store metadata next to RenderGraphResource
                renderPassDesc.depthAttachment.storeAction = GetStoreActionForRenderTexture(renderTexture, node, pass); // maybe store metadata next to RenderGraphResource
                renderPassDesc.depthAttachment.clearDepth = node->descriptor.clearDepth; // maybe store metadata next to RenderGraphResource
                renderPassDesc.depthAttachment.clearStencil = node->descriptor.clearStencil; // maybe store metadata next to RenderGraphResource
                
                const auto& descriptor = renderPassDesc.depthAttachment.texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            cmd->BeginRenderPass(renderPassDesc, pass->name);
			cmd->SetViewport(renderPassDesc.size);
            std::invoke(pass->callback, cmd);
            cmd->EndRenderPass();
        }
        
        // release resources that are not referenced in later passes
        mStackAllocator.
    }
    cmd->End();
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

TextureHandle RenderGraph::ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer)
{
    uint32_t id = static_cast<uint32_t>(mRegistry.mNodes.size());
    TextureHandle resource(id); // TODO: Assign backbuffer to the handle
    mRegistry.mNodes.emplace_back(id, resource, false);
    return resource;
}

const BufferDescriptor& RenderGraph::GetDescriptor(BufferHandle handle) const
{
    const auto& node = mRegistry.GetResourceNode(handle);
}

const TextureDescriptor& RenderGraph::GetDescriptor(TextureHandle handle) const
{
    
}
