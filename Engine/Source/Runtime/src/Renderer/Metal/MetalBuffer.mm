#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

void Buffer::Allocate()
{
    switch (mMemoryType)
    {
        case MemoryType::Static:
        {
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:mSize options:MTLResourceStorageModePrivate];
            break;
        }
        case MemoryType::Dynamic:
        {
#if defined(PLATFORM_MACOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:mSize options:MTLResourceStorageModeManaged];
#elif defined(PLATFORM_IOS)
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:mSize options:MTLResourceStorageModeShared];
#endif
            break;
        }
        case MemoryType::Stream:
        {
            mHandle = [MetalDevice::GetHandle() newBufferWithLength:mSize options:MTLResourceStorageModeShared];
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "Metal: Unknown memory type given!");
            break;
        }
    }
    
    if (mMemoryType != MemoryType::Static)
    {
        mContents = [id<MTLBuffer>(mHandle) contents];
    }
}

void Buffer::Free()
{
    mHandle = nil;
}
#endif
