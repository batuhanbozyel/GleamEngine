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
    
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor, bool transient = true);
    
    NO_DISCARD TextureHandle CreateTexture(const RenderTextureDescriptor& descriptor, bool transient = true);
    
private:
    
    Deque<RenderGraphBufferNode> mBufferNodes;

    Deque<RenderGraphTextureNode> mTextureNodes;
    
};

} // namespace Gleam

