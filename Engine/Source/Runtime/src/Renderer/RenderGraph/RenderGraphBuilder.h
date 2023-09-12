#pragma once
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Gleam {

class RenderGraphBuilder final
{
public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

    RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry);

    NO_DISCARD TextureHandle UseColorBuffer(TextureHandle attachment);
    
    NO_DISCARD TextureHandle UseDepthBuffer(TextureHandle attachment);
    
    // RenderTexure
    NO_DISCARD TextureHandle CreateTexture(const RenderTextureDescriptor& descriptor);
    
    NO_DISCARD TextureHandle WriteTexture(TextureHandle resource);
    
    TextureHandle ReadTexture(TextureHandle resource);
    
    // Buffer
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle WriteBuffer(const BufferHandle& resource);
    
    BufferHandle ReadBuffer(const BufferHandle& resource);

private:
	
	RenderPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
