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
        case MemoryType::GPU:
        {
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModePrivate];
            break;
        }
        case MemoryType::Shared:
        {
#if defined(PLATFORM_MACOS) && defined(__arm64__)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeShared];
#elif defiend(PLATFORM_MACOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeManaged];
#elif defined(PLATFORM_IOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:descriptor.size options:MTLResourceStorageModeShared];
#endif
            break;
        }
        case MemoryType::CPU:
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
    
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        mContents = [id<MTLBuffer>(mHandle) contents];
    }
}

Buffer::~Buffer()
{
    mHandle = nil;
    mContents = nullptr;
}

#endif
