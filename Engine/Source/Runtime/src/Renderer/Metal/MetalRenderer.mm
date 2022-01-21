#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Renderer.h"
#include "MetalUtils.h"

using namespace Gleam;

void Renderer::BeginFrame()
{
    const auto& frame = mContext->AcquireNextFrame();
    
    MTLRenderPassDescriptor* renderPassDesc = [MTLRenderPassDescriptor renderPassDescriptor];
    MTLRenderPassColorAttachmentDescriptor* colorAttachmentDesc = renderPassDesc.colorAttachments[0];
    colorAttachmentDesc.clearColor = MTLClearColorMake(0.0f, 0.0f, 0.0f, 0.0f);
    colorAttachmentDesc.loadAction = MTLLoadActionClear;
    colorAttachmentDesc.storeAction = MTLStoreActionStore;
    colorAttachmentDesc.texture = frame.drawable.texture;
    
    id<MTLRenderCommandEncoder> renderCommandEncoder = [frame.commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];
    [renderCommandEncoder endEncoding];
}

void Renderer::EndFrame()
{
    mContext->Present();
}

#endif
