//
//  RenderGraphResourceRegistry.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 12.03.2023.
//

#pragma once
#include "RenderGraphNode.h"

namespace Gleam {

class RenderGraphResourceRegistry final
{
public:
    
    void Clear();
    
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);
    
    NO_DISCARD TextureHandle CreateRT(const RenderTextureDescriptor& descriptor);
    
    RenderGraphResourceNode& GetResourceNode(ResourceHandle resource);
    
    const RenderGraphResourceNode& GetResourceNode(ResourceHandle resource) const;
    
private:
    
    TArray<RenderGraphResourceNode> mNodes;
    
};

} // namespace Gleam

