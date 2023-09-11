#pragma once
#include "RenderGraphNode.h"
#include "RenderGraphResourceRegistry.h"

namespace Gleam {

static bool HasResource(const TArray<ResourceHandle>& resources, ResourceHandle resource)
{
    return std::find(resources.cbegin(), resources.cend(), resource) != resources.cend();
}

template <typename T>
concept ResourceType = std::is_base_of<ResourceHandle, T>::value;

class RenderGraphBuilder final
{
public:

	GLEAM_NONCOPYABLE(RenderGraphBuilder);

    RenderGraphBuilder(RenderPassNode& node, RenderGraphResourceRegistry& registry);

    NO_DISCARD TextureHandle UseColorBuffer(TextureHandle attachment);
    
    NO_DISCARD TextureHandle UseDepthBuffer(TextureHandle attachment);
    
    // RenderTexure
    NO_DISCARD TextureHandle CreateRenderTexture(const RenderTextureDescriptor& descriptor);
    
    NO_DISCARD TextureHandle WriteRenderTexture(TextureHandle resource);
    
    TextureHandle ReadRenderTexture(TextureHandle resource);
    
    // Buffer
    NO_DISCARD BufferHandle CreateBuffer(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle WriteBuffer(const BufferHandle& resource);
    
    BufferHandle ReadBuffer(const BufferHandle& resource);

private:
    
    template<ResourceType T>
    NO_DISCARD T WriteResource(const T& resource)
    {
        auto& node = mResourceRegistry.GetResourceNode(resource);
        if (node.transient == false) { mPassNode.hasSideEffect = true; }
        
        if (HasResource(mPassNode.resourceWrites, resource)) { return resource; }
        
        node.producers.push_back(&mPassNode);
        
        T clone = T(resource, resource.GetVersion() + 1, ResourceAccess::Write);
        mPassNode.resourceWrites.emplace_back(clone);
        return clone;
    }
	
	RenderPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
