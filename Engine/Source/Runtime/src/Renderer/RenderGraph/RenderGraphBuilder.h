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
    NO_DISCARD RenderTextureHandle Create(const RenderTextureDescriptor& descriptor);
    
    NO_DISCARD RenderTextureHandle Write(RenderTextureHandle resource);
    
    RenderTextureHandle Read(RenderTextureHandle resource);
    
    // Buffer
    NO_DISCARD BufferHandle Create(const BufferDescriptor& descriptor);

    NO_DISCARD BufferHandle Write(BufferHandle resource);
    
    BufferHandle Read(BufferHandle resource);

private:
	
	RenderPassNode& mPassNode;
    
    RenderGraphResourceRegistry& mResourceRegistry;
};

} // namespace Gleam
