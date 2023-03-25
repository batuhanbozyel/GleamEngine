#pragma once
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Gleam {

class RenderGraphBuilder final
{
public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

    RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry);

    // RenderTexure
    NO_DISCARD RenderTextureHandle CreateRenderTexture(const TextureDescriptor& descriptor);
    
    NO_DISCARD RenderTextureHandle WriteRenderTexture(RenderTextureHandle resource);
    
    RenderTextureHandle ReadRenderTexture(RenderTextureHandle resource);
    
    // Buffer
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle WriteBuffer(BufferHandle resource);
    
    BufferHandle ReadBuffer(BufferHandle resource);

private:
	
	RenderPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
