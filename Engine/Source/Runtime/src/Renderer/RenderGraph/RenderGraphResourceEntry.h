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
    
    RenderPassNode* creator = nullptr;
    RenderPassNode* lastModifier = nullptr;
    RenderPassNode* lastReference = nullptr;
    
    uint32_t refCount = 0;
    
    RenderGraphResourceEntry(RenderGraphResource resource, bool transient)
        : resource(resource), transient(transient)
    {
        
    }
    
    virtual ~RenderGraphResourceEntry() = default;
    
    virtual void Allocate() = 0;
    
    virtual void Release() = 0;
};

struct RenderGraphBufferEntry final : public RenderGraphResourceEntry
{
    BufferDescriptor descriptor;
    RefCounted<Buffer> buffer;
    
    RenderGraphBufferEntry(const BufferDescriptor& descriptor, RenderGraphResource resource, bool transient = true)
        : RenderGraphResourceEntry(resource, transient), descriptor(descriptor)
    {
        
    }
    
    virtual void Allocate() override
    {
        buffer = CreateRef<Buffer>(descriptor);
    }
    
    virtual void Release() override
    {
        buffer.reset();
    }
};

struct RenderGraphRenderTextureEntry final : public RenderGraphResourceEntry
{
    RenderTextureDescriptor descriptor;
    RefCounted<RenderTexture> renderTexture;
    
    RenderGraphRenderTextureEntry(const TextureDescriptor& descriptor, RenderGraphResource resource, bool transient = true)
        : RenderGraphResourceEntry(resource, transient), descriptor{.texture = descriptor}
    {
        
    }
    
    virtual void Allocate() override
    {
        renderTexture = CreateRef<RenderTexture>(descriptor.texture);
    }
    
    virtual void Release() override
    {
        renderTexture.reset();
    }
};

} // namespace Gleam
