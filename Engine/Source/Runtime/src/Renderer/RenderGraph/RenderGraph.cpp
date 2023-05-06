#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"

using namespace Gleam;

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
            mRegistry.GetResourceEntry(id)->producer = pass;
        for (auto id : pass->resourceWrites)
            mRegistry.GetResourceEntry(id)->last = pass;
        for (auto id : pass->resourceReads)
            mRegistry.GetResourceEntry(id)->last = pass;
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
                renderPassDesc.colorAttachments[i].texture = mRegistry.GetRenderTexture(attachment.texture);
                renderPassDesc.colorAttachments[i].loadAction = attachment.loadAction;
                renderPassDesc.colorAttachments[i].storeAction = attachment.storeAction;
                renderPassDesc.colorAttachments[i].clearColor = attachment.clearColor;
                
                const auto& descriptor = renderPassDesc.colorAttachments[i].texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            if (pass->depthAttachment.texture != RenderGraphResource::nullHandle)
            {
                renderPassDesc.depthAttachment.texture = mRegistry.GetRenderTexture(pass->depthAttachment.texture);;
                renderPassDesc.depthAttachment.loadAction = pass->depthAttachment.loadAction;
                renderPassDesc.depthAttachment.storeAction = pass->depthAttachment.storeAction;
                renderPassDesc.depthAttachment.clearDepth = pass->depthAttachment.clearDepth;
                
                const auto& descriptor = renderPassDesc.depthAttachment.texture->GetDescriptor();
                renderPassDesc.size = descriptor.size;
                renderPassDesc.samples = descriptor.sampleCount;
            }
            
            cmd->BeginRenderPass(renderPassDesc, pass->name);
            std::invoke(pass->callback, context);
            cmd->EndRenderPass();
        }
        
        // release pass resources
        // TODO: release from pools
        for (auto& entry : mRegistry.mEntries)
        {
            if (entry->last == pass)
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
