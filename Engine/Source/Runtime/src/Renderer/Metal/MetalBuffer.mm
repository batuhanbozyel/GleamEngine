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
#if defined(PLATFORM_MACOS)
            buffer.mHandle = [MetalDevice newBufferWithLength:buffer.mSize options:MTLResourceStorageModeManaged];
#elif defined(PLATFORM_IOS)
            buffer.mHandle = [MetalDevice newBufferWithLength:buffer.mSize options:MTLResourceStorageModeShared];
#endif
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

void Buffer::Copy(const Buffer& src, const Buffer& dst, uint32_t srcOffset, uint32_t dstOffset)
{
    GLEAM_ASSERT(src.GetSize() <= dst.GetSize(), "Metal: Source buffer size can not be larger than destination buffer size!");
    
    id<MTLCommandBuffer> commandBuffer = [RendererContext::GetTransferCommandPool(0) commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder copyFromBuffer:src.mHandle sourceOffset:srcOffset toBuffer:dst.mHandle destinationOffset:dstOffset size:src.mSize];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}
#endif
