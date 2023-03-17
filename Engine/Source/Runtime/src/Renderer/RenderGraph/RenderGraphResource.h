#pragma once
#include "Buffer.h"
#include "Texture.h"

namespace Gleam {

struct RenderGraphResource
{
    static const RenderGraphResource invalidHandle;
    
    uint32_t uniqueId;
    
    operator uint32_t() const
    {
        return uniqueId;
    }
};

struct BufferHandle : public RenderGraphResource {};
struct RenderTextureHandle : public RenderGraphResource {};

struct RenderGraphResourceEntry
{
    const RenderGraphResource resource;
    const bool transient;
    uint32_t version;
    
    RenderGraphResourceEntry(RenderGraphResource resource, uint32_t version, bool transient)
        : resource(resource), version(version), transient(transient)
    {
        
    }
};

struct RenderGraphBufferEntry : public RenderGraphResourceEntry
{
    BufferDescriptor descriptor;
    Scope<Buffer> buffer;
    
    RenderGraphBufferEntry(const BufferDescriptor& descriptor, RenderGraphResource resource, uint32_t version, bool transient = true)
        : RenderGraphResourceEntry(resource, version, transient), descriptor(descriptor)
    {
        
    }
};

struct RenderGraphRenderTextureEntry : public RenderGraphResourceEntry
{
    RenderTextureDescriptor descriptor;
    Scope<RenderTexture> renderTexture;
    
    RenderGraphRenderTextureEntry(const RenderTextureDescriptor& descriptor, RenderGraphResource resource, uint32_t version, bool transient = true)
        : RenderGraphResourceEntry(resource, version, transient), descriptor(descriptor)
    {
        
    }
};

} // namespace Gleam
