#pragma once
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Gleam {

static bool HasResource(const TArray<RenderGraphResource>& resources, RenderGraphResource resource)
{
    return std::find(resources.cbegin(), resources.cend(), resource) != resources.cend();
}

class RenderGraphBuilder final
{
public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

    RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry);

    NO_DISCARD RenderTextureHandle UseColorBuffer(RenderGraphResource attachment);
    
    NO_DISCARD RenderTextureHandle UseDepthBuffer(RenderGraphResource attachment);
    
    // RenderTexure
    NO_DISCARD RenderTextureHandle CreateRenderTexture(const TextureDescriptor& descriptor);
    
    NO_DISCARD RenderTextureHandle WriteRenderTexture(RenderTextureHandle resource);
    
    RenderTextureHandle ReadRenderTexture(RenderTextureHandle resource);
    
    // Buffer
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle WriteBuffer(BufferHandle resource);
    
    BufferHandle ReadBuffer(BufferHandle resource);

private:
    
    NO_DISCARD RenderGraphResource WriteResource(RenderGraphResource resource);
	
	RenderPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
