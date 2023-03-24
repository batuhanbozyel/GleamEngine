#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Buffer::Buffer(const BufferDescriptor& descriptor)
    : mDescriptor(descriptor)
{
    switch (descriptor.memoryType)
    {
        case MemoryType::Static:
        {
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModePrivate];
            break;
        }
        case MemoryType::Dynamic:
        {
#if defined(PLATFORM_MACOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeManaged];
#elif defined(PLATFORM_IOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeShared];
#endif
            break;
        }
        case MemoryType::Stream:
        {
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeShared];
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "Metal: Unknown memory type given!");
            break;
        }
    }
    
    if (mDescriptor.memoryType != MemoryType::Static)
    {
        mContents = [id<MTLBuffer>(mHandle) contents];
    }
}

Buffer::~Buffer()
{
    mHandle = nil;
    
    if (mDescriptor.memoryType != MemoryType::Static)
    {
        mContents = nullptr;
    }
}

#endif
