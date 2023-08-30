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

class RenderGraphResourceRegistry final
{
    friend class RenderGraph;
    friend class RenderGraphBuilder;
    
public:
    
    void Clear();
    
private:
    
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);
    
    NO_DISCARD RenderTextureHandle CreateRT(const TextureDescriptor& descriptor);
    
    NO_DISCARD RenderGraphResource CloneResource(RenderGraphResource resource);
    
    RenderGraphResourceEntry& GetResourceEntry(RenderGraphResource resource);
    
    RenderGraphResourceNode& GetResourceNode(RenderGraphResource resource);
    
    const RenderGraphResourceNode& GetResourceNode(RenderGraphResource resource) const;
    
    TArray<RenderGraphResourceNode> mNodes;
    TArray<RenderGraphResourceEntry> mEntries;
    
};

} // namespace Gleam

