#include "gpch.h"
#include "RenderGraph.h"
#include "Renderer/CommandBuffer.h"

using namespace Gleam;

static RenderGraphResourceNode& GetResourceNode(TArray<RenderGraphResourceNode>& nodes, RenderGraphResource resource)
{
    GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
    return nodes[resource];
}

void RenderGraph::Compile()
{
    for (auto pass : mPassNodes)
    {
        // Buffer
        pass->refCount += pass->bufferWrites.size();
        for (auto resource : pass->bufferReads)
        {
            auto& consumed = GetResourceNode(mRegistry.mBufferNodes, resource);
            consumed.refCount++;
        }
        for (auto resource : pass->bufferWrites)
        {
            auto& written = GetResourceNode(mRegistry.mBufferNodes, resource);
            written.producer = pass;
        }
        
        // RenderTexture
        pass->refCount += pass->renderTextureWrites.size();
        for (auto resource : pass->renderTextureReads)
        {
            auto& consumed = GetResourceNode(mRegistry.mRenderTextureNodes, resource);
            consumed.refCount++;
        }
        for (auto resource : pass->renderTextureWrites)
        {
            auto& written = GetResourceNode(mRegistry.mRenderTextureNodes, resource);
            written.producer = pass;
        }
    }
    
    // Cull render passes
    // Buffer
    Stack<RenderGraphResourceNode*> unreferencedBuffers;
    for (auto& node : mRegistry.mBufferNodes)
    {
        if (node.refCount == 0)
        {
            unreferencedBuffers.push(&node);
        }
    }
    
    while (!unreferencedBuffers.empty())
    {
        auto buffer = unreferencedBuffers.top();
        unreferencedBuffers.pop();
        
        auto producer = buffer->producer;
        if (producer == nullptr || producer->hasSideEffect) continue;
        
        if (--producer->refCount == 0)
        {
            for (auto resource : producer->bufferReads)
            {
                auto& node = GetResourceNode(mRegistry.mBufferNodes, resource);
                if (--node.refCount == 0) unreferencedBuffers.push(&node);
            }
        }
    }
    
    // RenderTexture
    Stack<RenderGraphResourceNode*> unreferencedRenderTextures;
    for (auto& node : mRegistry.mRenderTextureNodes)
    {
        if (node.refCount == 0)
        {
            unreferencedRenderTextures.push(&node);
        }
    }
    
    while (!unreferencedRenderTextures.empty())
    {
        auto renderTexture = unreferencedRenderTextures.top();
        unreferencedRenderTextures.pop();
        
        auto producer = renderTexture->producer;
        if (producer == nullptr || producer->hasSideEffect) continue;
        
        if (--producer->refCount == 0)
        {
            for (auto resource : producer->renderTextureReads)
            {
                auto& node = GetResourceNode(mRegistry.mRenderTextureNodes, resource);
                if (--node.refCount == 0) unreferencedRenderTextures.push(&node);
            }
        }
    }
    
    // Calculate resource lifetimes
    for (auto pass : mPassNodes)
    {
        if (pass->isCulled()) continue;
        
        // Buffer resources
        for (auto id : pass->bufferCreates)
            mRegistry.GetBufferEntry(id).producer = pass;
        for (auto id : pass->bufferWrites)
            mRegistry.GetBufferEntry(id).last = pass;
        for (auto id : pass->bufferReads)
            mRegistry.GetBufferEntry(id).last = pass;
        
        // RenderTexture resources
        for (auto id : pass->renderTextureCreates)
            mRegistry.GetRenderTextureEntry(id).producer = pass;
        for (auto id : pass->renderTextureWrites)
            mRegistry.GetRenderTextureEntry(id).last = pass;
        for (auto id : pass->renderTextureReads)
            mRegistry.GetRenderTextureEntry(id).last = pass;
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
        if (pass->isCulled()) continue;
        
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
        for (auto& entry : mRegistry.mBufferEntries)
        {
            if (entry.last == pass)
                entry.buffer.reset();
        }
        
        for (auto& entry : mRegistry.mRenderTextureEntries)
        {
            if (entry.last == pass)
                entry.renderTexture.reset();
        }
    }
    cmd->End();
    
    for (auto pass : mPassNodes) { delete pass; }
    mPassNodes.clear();
    mRegistry.Clear();
}

RenderTextureHandle RenderGraph::ImportBackbuffer(const RefCounted<RenderTexture>& backbuffer)
{
    RenderTextureHandle resource(static_cast<uint32_t>(mRegistry.mRenderTextureEntries.size()));
    auto& entry = mRegistry.mRenderTextureEntries.emplace_back(backbuffer->GetDescriptor(), resource, false);
    entry.renderTexture = backbuffer;
    
    RenderTextureHandle node(static_cast<uint32_t>(mRegistry.mRenderTextureNodes.size()));
    mRegistry.mRenderTextureNodes.emplace_back(node, resource);
    return node;
}
