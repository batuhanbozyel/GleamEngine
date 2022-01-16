#pragma once
#ifdef USE_METAL_RENDERER
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

#define MetalDevice id<MTLDevice>(Renderer::GetDevice())

struct MetalFrameObject
{
    id<CAMetalDrawable> drawable;
    id<MTLCommandBuffer> commandBuffer;
};
    
} // namespace Gleam
#endif
