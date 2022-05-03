#pragma once
#ifdef USE_METAL_RENDERER
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "Renderer/RendererContext.h"
#include "Renderer/Renderer.h"

namespace Gleam {
    
#define MetalDevice id<MTLDevice>(RendererContext::GetDevice())

struct MetalFrameObject
{
    id<CAMetalDrawable> drawable{ nil };
    dispatch_semaphore_t imageAcquireSemaphore;
};

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
    
    
} // namespace Gleam
#endif
