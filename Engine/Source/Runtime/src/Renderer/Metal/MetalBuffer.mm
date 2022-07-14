#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

void Buffer::Allocate(Buffer& buffer)
{
    switch (buffer.mMemoryType)
    {
        case MemoryType::Static:
        {
            buffer.mHandle = [MetalDevice newBufferWithLength:buffer.mSize options:MTLResourceStorageModePrivate];
            break;
        }
        case MemoryType::Dynamic:
        {
            buffer.mHandle = [MetalDevice newBufferWithLength:buffer.mSize options:MTLResourceStorageModeManaged];
            break;
        }
        case MemoryType::Stream:
        {
            buffer.mHandle = [MetalDevice newBufferWithLength:buffer.mSize options:MTLResourceStorageModeShared];
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "Vulkan: Unknown memory type given!");
            break;
        }
    }
    
    if (buffer.mMemoryType != MemoryType::Static)
    {
        buffer.mContents = [id<MTLBuffer>(buffer.mHandle) contents];
    }
}

void Buffer::Free(Buffer& buffer)
{
    buffer.mHandle = nil;
}

void Buffer::Copy(const Buffer& src, const Buffer& dst)
{
    GLEAM_ASSERT(src.GetSize() <= dst.GetSize(), "Metal: Source buffer size can not be larger than destination buffer size!");
    
    id<MTLCommandBuffer> commandBuffer = [RendererContext::GetGraphicsCommandPool(0) commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder copyFromBuffer:src.mHandle sourceOffset:0 toBuffer:dst.mHandle destinationOffset:0 size:src.mSize];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}
#endif
