#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Buffer::Buffer(uint32_t size, BufferUsage usage)
    : mSize(size), mUsage(usage)
{
    mHandle = [MetalDevice newBufferWithLength:size options:MTLResourceStorageModeShared];
    mContents = [id<MTLBuffer>(mHandle) contents];
}

Buffer::Buffer(const void* data, uint32_t size, BufferUsage usage)
    : mSize(size)
{
    mHandle = [MetalDevice newBufferWithBytes:data length:size options:MTLResourceStorageModeShared];
    mContents = [id<MTLBuffer>(mHandle) contents];
}

Buffer::~Buffer()
{
    mHandle = nil;
}

void Buffer::SetData(const void* data, uint32_t offset, uint32_t size) const
{
    memcpy(As<uint8_t*>(mContents) + offset, data, size);
}
#endif
