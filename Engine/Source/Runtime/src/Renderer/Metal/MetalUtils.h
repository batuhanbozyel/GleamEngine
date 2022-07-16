#pragma once
#ifdef USE_METAL_RENDERER
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "Renderer/RendererContext.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {
    
#define MetalDevice id<MTLDevice>(RendererContext::GetDevice())

static constexpr TextureFormat MTLPixelFormatToTextureFormat(MTLPixelFormat format)
{
    switch (format)
    {
        // TODO:
        default: return TextureFormat::None;
    }
}

static constexpr MTLPixelFormat TextureFormatToVkFormat(TextureFormat format)
{
    switch (format)
    {
        // TODO:
        default: return MTLPixelFormatInvalid;
    }
}

static constexpr MTLPrimitiveType PrimitiveToplogyToMTLPrimitiveType(PrimitiveTopology topology)
{
    switch (topology)
    {
        case PrimitiveTopology::Points: return MTLPrimitiveTypePoint;
        case PrimitiveTopology::Lines: return MTLPrimitiveTypeLine;
        case PrimitiveTopology::LineStrip: return MTLPrimitiveTypeLineStrip;
        case PrimitiveTopology::Triangles: return MTLPrimitiveTypeTriangle;
        default: GLEAM_ASSERT(false, "Metal: Unknown primitive topology specified!"); return MTLPrimitiveType(~0);
    }
}
    
} // namespace Gleam
#endif
