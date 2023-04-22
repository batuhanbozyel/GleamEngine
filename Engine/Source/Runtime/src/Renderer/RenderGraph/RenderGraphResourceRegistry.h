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
    
public:
    
    void Clear();
    
    const RefCounted<Buffer>& GetBuffer(BufferHandle buffer) const;
    
    const RefCounted<RenderTexture>& GetRenderTexture(RenderTextureHandle renderTexture) const;
    
private:
    
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);
    
    NO_DISCARD RenderTextureHandle CreateRT(const TextureDescriptor& descriptor);
    
    NO_DISCARD BufferHandle CloneBuffer(BufferHandle resource);
    
    NO_DISCARD RenderTextureHandle CloneRT(RenderTextureHandle resource);
    
    RenderGraphBufferEntry& GetBufferEntry(RenderGraphResource resource);
    
    RenderGraphRenderTextureEntry& GetRenderTextureEntry(RenderGraphResource resource);
    
    TArray<RenderGraphResourceNode> mRenderTextureNodes;
    TArray<RenderGraphRenderTextureEntry> mRenderTextureEntries;
    
    TArray<RenderGraphResourceNode> mBufferNodes;
    TArray<RenderGraphBufferEntry> mBufferEntries;
    
};

} // namespace Gleam

