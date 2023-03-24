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
    uint32_t version;
    
    RenderPassNode* producer = nullptr;
    RenderPassNode* last = nullptr;
    
    RenderGraphResourceEntry(RenderGraphResource resource, uint32_t version, bool transient)
        : resource(resource), version(version), transient(transient)
    {
        
    }
};

struct RenderGraphBufferEntry : public RenderGraphResourceEntry
{
    BufferDescriptor descriptor;
    RefCounted<Buffer> buffer;
    
    RenderGraphBufferEntry(const BufferDescriptor& descriptor, RenderGraphResource resource, uint32_t version, bool transient = true)
        : RenderGraphResourceEntry(resource, version, transient), descriptor(descriptor)
    {
        
    }
};

struct RenderGraphRenderTextureEntry : public RenderGraphResourceEntry
{
    TextureDescriptor descriptor;
    RefCounted<RenderTexture> renderTexture;
    
    RenderGraphRenderTextureEntry(const TextureDescriptor& descriptor, RenderGraphResource resource, uint32_t version, bool transient = true)
        : RenderGraphResourceEntry(resource, version, transient), descriptor(descriptor)
    {
        
    }
};

} // namespace Gleam
