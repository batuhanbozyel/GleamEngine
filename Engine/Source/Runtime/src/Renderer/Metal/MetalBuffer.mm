#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

void IBuffer::Allocate(IBuffer& buffer)
{
    switch (buffer.mMemoryType)
    {
        case MemoryType::Static:
        {
            buffer.mHandle = [MetalDevice::GetHandle() newBufferWithLength:buffer.mSize options:MTLResourceStorageModePrivate];
            break;
        }
        case MemoryType::Dynamic:
        {
#if defined(PLATFORM_MACOS)
            buffer.mHandle = [MetalDevice::GetHandle() newBufferWithLength:buffer.mSize options:MTLResourceStorageModeManaged];
#elif defined(PLATFORM_IOS)
            buffer.mHandle = [MetalDevice::GetHandle() newBufferWithLength:buffer.mSize options:MTLResourceStorageModeShared];
#endif
            break;
        }
        case MemoryType::Stream:
        {
            buffer.mHandle = [MetalDevice::GetHandle() newBufferWithLength:buffer.mSize options:MTLResourceStorageModeShared];
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "Metal: Unknown memory type given!");
            break;
        }
    }
    
    if (buffer.mMemoryType != MemoryType::Static)
    {
        buffer.mContents = [id<MTLBuffer>(buffer.mHandle) contents];
    }
}

void IBuffer::Free(IBuffer& buffer)
{
    buffer.mHandle = nil;
}

void IBuffer::Copy(const IBuffer& src, const IBuffer& dst, size_t srcOffset, size_t dstOffset)
{
    GLEAM_ASSERT(src.GetSize() <= dst.GetSize(), "Metal: Source buffer size can not be larger than destination buffer size!");
    
    id<MTLCommandBuffer> commandBuffer = [MetalDevice::GetCommandPool() commandBuffer];
    id<MTLBlitCommandEncoder> commandEncoder = [commandBuffer blitCommandEncoder];
    
    [commandEncoder copyFromBuffer:src.mHandle sourceOffset:srcOffset toBuffer:dst.mHandle destinationOffset:dstOffset size:src.mSize];
    [commandEncoder endEncoding];
    
    [commandBuffer commit];
}
#endif
