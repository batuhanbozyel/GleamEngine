//
//  RenderGraphResourceEntry.h
//  GleamEngine
//
//  Created by Batuhan Bozyel on 24.03.2023.
//
#pragma once
#include "RenderGraphNode.h"

#include "Renderer/Buffer.h"
#include "Renderer/Texture.h"

namespace Gleam {

struct RenderGraphResourceEntry
{
    const RenderGraphResource resource;
    const bool transient;
    
    RenderPassNode* producer = nullptr;
    RenderPassNode* last = nullptr;
    
    RenderGraphResourceEntry(RenderGraphResource resource, bool transient)
        : resource(resource), transient(transient)
    {
        
    }
};

struct RenderGraphBufferEntry : public RenderGraphResourceEntry
{
    BufferDescriptor descriptor;
    RefCounted<Buffer> buffer;
    
    RenderGraphBufferEntry(const BufferDescriptor& descriptor, RenderGraphResource resource, bool transient = true)
        : RenderGraphResourceEntry(resource, transient), descriptor(descriptor)
    {
        
    }
};

struct RenderGraphRenderTextureEntry : public RenderGraphResourceEntry
{
    TextureDescriptor descriptor;
    RefCounted<RenderTexture> renderTexture;
    
    RenderGraphRenderTextureEntry(const TextureDescriptor& descriptor, RenderGraphResource resource, bool transient = true)
        : RenderGraphResourceEntry(resource, transient), descriptor(descriptor)
    {
        
    }
};

} // namespace Gleam
