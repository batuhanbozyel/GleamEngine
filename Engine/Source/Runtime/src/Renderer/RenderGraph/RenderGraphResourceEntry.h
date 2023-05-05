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
    TextureDescriptor descriptor;
    RefCounted<RenderTexture> renderTexture;
    
    RenderGraphRenderTextureEntry(const TextureDescriptor& descriptor, RenderGraphResource resource, bool transient = true)
        : RenderGraphResourceEntry(resource, transient), descriptor(descriptor)
    {
        
    }
    
    virtual void Allocate() override
    {
        renderTexture = CreateRef<RenderTexture>(descriptor);
    }
    
    virtual void Release() override
    {
        renderTexture.reset();
    }
};

} // namespace Gleam
