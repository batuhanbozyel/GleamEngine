#pragma once
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Gleam {

template <typename T>
concept ResourceType = std::is_base_of<ResourceHandle, T>::value;

template <ResourceType T>
static bool HasResource(const TArray<T>& resources, T resource)
{
    return std::find(resources.cbegin(), resources.cend(), resource) != resources.cend();
}

class RenderGraphBuilder final
{
public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

    RenderGraphBuilder(RenderGraphPassNode& node, RenderGraphResourceRegistry& registry);

    NO_DISCARD TextureHandle UseColorBuffer(const TextureHandle& attachment);
    
    NO_DISCARD TextureHandle UseDepthBuffer(const TextureHandle& attachment);
    
    // RenderTexure
    NO_DISCARD TextureHandle CreateTexture(const RenderTextureDescriptor& descriptor);
    
    NO_DISCARD TextureHandle WriteTexture(const TextureHandle& resource);
    
    NO_DISCARD TextureHandle ReadTexture(const TextureHandle& resource);
    
    // Buffer
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle WriteBuffer(const BufferHandle& resource);
    
    NO_DISCARD BufferHandle ReadBuffer(const BufferHandle& resource);

private:
	
	RenderGraphPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
