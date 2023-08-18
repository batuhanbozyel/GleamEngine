#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"

using namespace Gleam;

static AttachmentLoadAction GetLoadActionForRenderTexture(const RefCounted<RenderTexture>& renderTexture, RenderGraphRenderTextureEntry* entry, RenderPassNode* pass)
{
    if (entry->creator == pass || !entry->transient)
    {
        return entry->descriptor.clearBuffer ? AttachmentLoadAction::Clear : AttachmentLoadAction::DontCare;
    }
    return AttachmentLoadAction::Load;
}

static AttachmentStoreAction GetStoreActionForRenderTexture(const RefCounted<RenderTexture>& renderTexture, RenderGraphRenderTextureEntry* entry, RenderPassNode* pass)
{
    if (renderTexture->GetDescriptor().sampleCount > 1)
    {
        return entry->lastModifier == pass ? AttachmentStoreAction::Resolve : AttachmentStoreAction::StoreAndResolve;
    }
    return (entry->lastReference == pass && !pass->hasSideEffect) ? AttachmentStoreAction::DontCare : AttachmentStoreAction::Store;
}

void RenderGraph::Compile()
{
    // Setup resource dependency
    for (auto pass : mPassNodes)
    {
        for (auto resource : pass->resourceReads)
        {
            auto& consumed = mRegistry.GetResourceNode(resource);
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
        for (auto id : pass->resourceCreates)
        {
            auto entry = mRegistry.GetResourceEntry(id);
            entry->creator = pass;
            entry->lastModifier = pass;
            entry->lastReference = pass;
        }
        for (auto id : pass->resourceWrites)
        {
            auto entry = mRegistry.GetResourceEntry(id);
            entry->lastModifier = pass;
            entry->lastReference = pass;
        }
        for (auto id : pass->resourceReads)
        {
            mRegistry.GetResourceEntry(id)->lastReference = pass;
        }
    }
}

void RenderGraph::Execute(const CommandBuffer* cmd)
{
    RenderGraphContext context;
    context.cmd = cmd;
    context.registry = &mRegistry;
    
    cmd->Begin();
    for (auto& pass : mPassNodes)
    {
        // allocate pass resources
        // TODO: use allocators to avoid allocating GPU resources on the fly
        for (auto id : pass->resourceCreates)
        {
            auto entry = mRegistry.GetResourceEntry(id);
            entry->Allocate();
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
                const auto& attachment = pass->colorAttachments[i];
                const auto& renderTexture = mRegistry.GetRenderTexture(attachment);
                renderPassDesc.colorAttachments[i].texture = renderTexture;
                
                auto entry = static_cast<RenderGraphRenderTextureEntry*>(mRegistry.GetResourceEntry(attachment));
                renderPassDesc.colorAttachments[i].loadAction = GetLoadActionForRenderTexture(renderTexture, entry, pass);
                renderPassDesc.colorAttachments[i].storeAction = GetStoreActionForRenderTexture(renderTexture, entry, pass);
                renderPassDesc.colorAttachments[i].clearColor = entry->descriptor.clearColor;
                
                const auto& descriptor = renderPassDesc.colorAttachments[i].texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            if (pass->depthAttachment != RenderGraphResource::nullHandle)
            {
                const auto& renderTexture = mRegistry.GetRenderTexture(pass->depthAttachment);
                renderPassDesc.depthAttachment.texture = renderTexture;
                
                auto entry = static_cast<RenderGraphRenderTextureEntry*>(mRegistry.GetResourceEntry(pass->depthAttachment));
                renderPassDesc.depthAttachment.loadAction = GetLoadActionForRenderTexture(renderTexture, entry, pass);
                renderPassDesc.depthAttachment.storeAction = GetStoreActionForRenderTexture(renderTexture, entry, pass);
                renderPassDesc.depthAttachment.clearDepth = entry->descriptor.clearDepth;
                renderPassDesc.depthAttachment.clearStencil = entry->descriptor.clearStencil;
                
                const auto& descriptor = renderPassDesc.depthAttachment.texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            cmd->BeginRenderPass(renderPassDesc, pass->name);
			cmd->SetViewport(renderPassDesc.size);
            std::invoke(pass->callback, context);
            cmd->EndRenderPass();
        }
        
        // release pass resources
        // TODO: release from pools
        for (auto& entry : mRegistry.mEntries)
        {
            if (entry->lastReference == pass)
                entry->Release();
        }
    }
    cmd->End();
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

RenderTextureHandle RenderGraph::ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mRegistry.mEntries.size()));
    auto entry = mRegistry.mEntries.emplace_back(CreateScope<RenderGraphRenderTextureEntry>(backbuffer->GetDescriptor(), resource, false)).get();
    static_cast<RenderGraphRenderTextureEntry*>(entry)->renderTexture = backbuffer;
    
    RenderTextureHandle node(static_cast<uint32_t>(mRegistry.mNodes.size()));
    mRegistry.mNodes.emplace_back(node, resource);
    return node;
}

RenderTextureDescriptor& RenderGraph::GetDescriptor(RenderTextureHandle resource)
{
    return static_cast<RenderGraphRenderTextureEntry*>(mRegistry.GetResourceEntry(resource))->descriptor;
}
