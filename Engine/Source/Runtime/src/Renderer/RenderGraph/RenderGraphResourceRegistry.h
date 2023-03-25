//
//  RenderGraphResourceRegistry.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#pragma once
#include "RenderGraphResourceEntry.h"

namespace Gleam {

class RenderGraph;
class RenderGraphBuilder;

class RenderGraphResourceRegistry
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
    static constexpr uint32_t kInitialResourceVersion = 1;
    
public:
    
    void Clear()
    {
        mRenderTextureNodes.clear();
        mRenderTextureEntries.clear();
        
        mBufferNodes.clear();
        mBufferEntries.clear();
    }
    
    const RefCounted<Buffer>& GetBuffer(BufferHandle buffer) const
    {
        GLEAM_ASSERT(buffer != RenderGraphResource::nullHandle);
        
        const auto& node = mBufferNodes[buffer];
        return mBufferEntries[node.resource].buffer;
    }
    
    const RefCounted<RenderTexture>& GetRenderTexture(RenderTextureHandle renderTexture) const
    {
        GLEAM_ASSERT(renderTexture != RenderGraphResource::nullHandle);
        
        const auto& node = mRenderTextureNodes[renderTexture];
        return mRenderTextureEntries[node.resource].renderTexture;
    }
    
private:
    
    NO_DISCARD RenderTextureHandle CreateRT(const TextureDescriptor& descriptor)
    {
        RenderTextureHandle resource(static_cast<uint32_t>(mRenderTextureEntries.size()));
        mRenderTextureEntries.emplace_back(descriptor, resource, kInitialResourceVersion);
        
        RenderTextureHandle node(static_cast<uint32_t>(mRenderTextureNodes.size()));
        mRenderTextureNodes.emplace_back(node, resource, kInitialResourceVersion);
        return node;
    }
    
    NO_DISCARD RenderTextureHandle CloneRT(RenderTextureHandle resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        const auto& node = mRenderTextureNodes[resource];
        auto& entry = mRenderTextureEntries[node.resource];
        entry.version++;
        
        RenderTextureHandle clone(static_cast<uint32_t>(mRenderTextureNodes.size()));
        mRenderTextureNodes.emplace_back(clone, node.resource, entry.version);
        return clone;
    }
    
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor)
    {
        BufferHandle resource(static_cast<uint32_t>(mBufferEntries.size()));
        mBufferEntries.emplace_back(descriptor, resource, kInitialResourceVersion);
        
        BufferHandle node(static_cast<uint32_t>(mBufferNodes.size()));
        mBufferNodes.emplace_back(node, resource, kInitialResourceVersion);
        return node;
    }
    
    NO_DISCARD BufferHandle CloneBuffer(BufferHandle resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        const auto& node = mBufferNodes[resource];
        auto& entry = mBufferEntries[node.resource];
        entry.version++;
        
        BufferHandle clone(static_cast<uint32_t>(mBufferNodes.size()));
        mBufferNodes.emplace_back(clone, node.resource, entry.version);
        return clone;
    }
    
    RenderGraphResourceNode& GetRenderTextureNode(RenderGraphResource resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        return mRenderTextureNodes[resource];
    }
    
    RenderGraphRenderTextureEntry& GetRenderTextureEntry(RenderGraphResource resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        const auto& node = mRenderTextureNodes[resource];
        return mRenderTextureEntries[node.resource];
    }
    
    RenderGraphResourceNode& GetBufferNode(RenderGraphResource resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        return mBufferNodes[resource];
    }
    
    RenderGraphBufferEntry& GetBufferEntry(RenderGraphResource resource)
    {
        GLEAM_ASSERT(resource != RenderGraphResource::nullHandle);
        
        const auto& node = mBufferNodes[resource];
        return mBufferEntries[node.resource];
    }
    
    TArray<RenderGraphResourceNode> mRenderTextureNodes;
    TArray<RenderGraphRenderTextureEntry> mRenderTextureEntries;
    
    TArray<RenderGraphResourceNode> mBufferNodes;
    TArray<RenderGraphBufferEntry> mBufferEntries;
    
};

} // namespace Gleam

