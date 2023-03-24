#include "gpch.h"
#include "RenderGraph.h"

using namespace Gleam;

void RenderGraph::Compile()
{
    for (auto& pass : mPassNodes)
    {
        pass->refCount += pass->bufferWrites.size();
        for (auto resource : pass->bufferReads)
        {
            auto& consumed = mRegistry.GetBufferNode(resource);
            consumed.refCount++;
        }
        for (auto resource : pass->bufferWrites)
        {
            auto& written = mRegistry.GetBufferNode(resource);
            written.producer = pass.get();
        }
    }
    
    // Cull render passes
    Stack<RenderGraphResourceNode*> unreferencedBuffers;
    for (auto& node : mRegistry.mBufferNodes)
    {
        if (node.refCount == 0)
            unreferencedBuffers.push(&node);
    }
    
    while (!unreferencedBuffers.empty())
    {
        auto buffer = unreferencedBuffers.top();
        unreferencedBuffers.pop();
        
        RenderPassNode* producer = buffer->producer;
        if (producer == nullptr || producer->hasSideEffect) continue;
        
        if (--producer->refCount == 0)
        {
            for (auto id : producer->bufferReads)
            {
                auto& node = mRegistry.GetBufferNode(id);
                if (--node.refCount == 0) unreferencedBuffers.push(&node);
            }
        }
    }
    
    Stack<RenderGraphResourceNode*> unreferencedRenderTextures;
    for (auto& node : mRegistry.mRenderTextureNodes)
    {
        if (node.refCount == 0)
            unreferencedRenderTextures.push(&node);
    }
    
    while (!unreferencedRenderTextures.empty())
    {
        auto renderTexture = unreferencedRenderTextures.top();
        unreferencedRenderTextures.pop();
        
        RenderPassNode* producer = renderTexture->producer;
        if (producer == nullptr || producer->hasSideEffect) continue;
        
        if (--producer->refCount == 0)
        {
            for (auto id : producer->renderTextureReads)
            {
                auto& node = mRegistry.GetRenderTextureNode(id);
                if (--node.refCount == 0) unreferencedRenderTextures.push(&node);
            }
        }
    }
    
    // Calculate resource lifetimes
    for (auto& pass : mPassNodes)
    {
        if (pass->refCount == 0) continue;
        
        // Buffer resources
        for (auto id : pass->bufferCreates)
            mRegistry.GetBufferEntry(id).producer = pass.get();
        for (auto id : pass->bufferWrites)
            mRegistry.GetBufferEntry(id).last = pass.get();
        for (auto id : pass->bufferReads)
            mRegistry.GetBufferEntry(id).last = pass.get();
        
        // RenderTexture resources
        for (auto id : pass->renderTextureCreates)
            mRegistry.GetRenderTextureEntry(id).producer = pass.get();
        for (auto id : pass->renderTextureWrites)
            mRegistry.GetRenderTextureEntry(id).last = pass.get();
        for (auto id : pass->renderTextureReads)
            mRegistry.GetRenderTextureEntry(id).last = pass.get();
    }
}

void RenderGraph::Execute(const CommandBuffer& cmd)
{
    RenderGraphContext context;
    context.cmd = &cmd;
    context.registry = &mRegistry;
    
    for (auto& pass : mPassNodes)
    {
        if (pass->isCulled())
            continue;
        
        // allocate pass resources
        // TODO: use allocators to avoid allocating GPU resources on the fly
        for (auto id : pass->bufferCreates)
        {
            auto& entry = mRegistry.GetBufferEntry(id);
            entry.buffer = CreateRef<Buffer>(entry.descriptor);
        }
        
        for (auto id : pass->renderTextureCreates)
        {
            auto& entry = mRegistry.GetRenderTextureEntry(id);
            entry.renderTexture = CreateRef<RenderTexture>(entry.descriptor);
        }
        
        // execute render pass
        std::invoke(pass->callback, context);
        
        // release pass resources
        // TODO: release from pools
        for (auto& entry : mRegistry.mBufferEntries)
        {
            if (entry.last == pass.get() && entry.transient)
                entry.buffer.reset();
        }
        
        for (auto& entry : mRegistry.mRenderTextureEntries)
        {
            if (entry.last == pass.get() && entry.transient)
                entry.renderTexture.reset();
        }
    }
    
    mRegistry.Clear();
    mPassNodes.clear();
}

RenderTextureHandle RenderGraph::ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mRegistry.mRenderTextureEntries.size()));
    auto& entry = mRegistry.mRenderTextureEntries.emplace_back(backbuffer->GetDescriptor(), resource, RenderGraphResourceRegistry::kInitialResourceVersion);
    entry.renderTexture = backbuffer;
    
    RenderTextureHandle node(static_cast<uint32_t>(mRegistry.mRenderTextureNodes.size()));
    mRegistry.mRenderTextureNodes.emplace_back(node, resource, RenderGraphResourceRegistry::kInitialResourceVersion);
    return node;
}
