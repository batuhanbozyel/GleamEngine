//
//  RenderGraphResourceRegistry.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#pragma once
#include "RenderGraphNode.h"

namespace Gleam {

class RenderGraph;
class RenderGraphBuilder;

class RenderGraphResourceRegistry
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
    static constexpr uint32_t kInitialResourceVersion = 1;
    
public:
    
    const Buffer* GetBuffer(BufferHandle buffer) const
    {
        const auto& node = mBufferNodes[buffer];
        return mBufferEntries[node.resource].buffer.get();
    }
    
    const RenderTexture* GetRenderTexture(RenderTextureHandle renderTexture) const
    {
        const auto& node = mRenderTextureNodes[renderTexture];
        return mRenderTextureEntries[node.resource].renderTexture.get();
    }
    
private:
    
    NO_DISCARD RenderTextureHandle Create(const RenderTextureDescriptor& descriptor)
    {
        RenderTextureHandle resource;
        resource.uniqueId = static_cast<uint32_t>(mRenderTextureEntries.size());
        mRenderTextureEntries.emplace_back(descriptor, resource, kInitialResourceVersion);
        
        RenderTextureHandle node;
        node.uniqueId = static_cast<uint32_t>(mRenderTextureNodes.size());
        mRenderTextureNodes.emplace_back(node.uniqueId, resource, kInitialResourceVersion);
        return node;
    }
    
    NO_DISCARD RenderTextureHandle Clone(RenderTextureHandle resource)
    {
        const auto& node = mRenderTextureNodes[resource];
        auto& entry = mRenderTextureEntries[node.resource];
        entry.version++;
        
        RenderTextureHandle clone;
        clone.uniqueId = static_cast<uint32_t>(mRenderTextureNodes.size());
        mRenderTextureNodes.emplace_back(clone.uniqueId, node.resource, entry.version);
        return clone;
    }
    
    NO_DISCARD BufferHandle Create(const BufferDescriptor& descriptor)
    {
        BufferHandle resource;
        resource.uniqueId = static_cast<uint32_t>(mBufferEntries.size());
        mBufferEntries.emplace_back(descriptor, resource, kInitialResourceVersion);
        
        BufferHandle node;
        node.uniqueId = static_cast<uint32_t>(mBufferNodes.size());
        mBufferNodes.emplace_back(node.uniqueId, resource, kInitialResourceVersion);
        return node;
    }
    
    NO_DISCARD BufferHandle Clone(BufferHandle resource)
    {
        const auto& node = mBufferNodes[resource];
        auto& entry = mBufferEntries[node.resource];
        entry.version++;
        
        BufferHandle clone;
        clone.uniqueId = static_cast<uint32_t>(mBufferNodes.size());
        mBufferNodes.emplace_back(clone.uniqueId, node.resource, entry.version);
        return clone;
    }
    
    RenderGraphResourceNode& GetRenderTextureNode(RenderGraphResource resource)
    {
        return mRenderTextureNodes[resource];
    }
    
    const RenderGraphRenderTextureEntry& GetRenderTextureEntry(RenderGraphResource resource) const
    {
        const auto& node = mRenderTextureNodes[resource];
        return mRenderTextureEntries[node.resource];
    }
    
    RenderGraphResourceNode& GetBufferNode(RenderGraphResource resource)
    {
        return mBufferNodes[resource];
    }
    
    const RenderGraphBufferEntry& GetBufferEntry(RenderGraphResource resource) const
    {
        const auto& node = mBufferNodes[resource];
        return mBufferEntries[node.resource];
    }
    
    TArray<RenderGraphResourceNode> mRenderTextureNodes;
    TArray<RenderGraphRenderTextureEntry> mRenderTextureEntries;
    
    TArray<RenderGraphResourceNode> mBufferNodes;
    TArray<RenderGraphBufferEntry> mBufferEntries;
    
};

} // namespace Gleam

