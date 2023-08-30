#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "MetalUtils.h"

using namespace Gleam;

Heap::Heap(const HeapDescriptor& descriptor)
    : mDescriptor(descriptor)
{
    MTLHeapDescriptor* desc = [MTLHeapDescriptor new];
    switch (descriptor.memoryType)
    {
        case MemoryType::GPU:
        {
            desc.resourceOptions = MTLResourceStorageModePrivate;
            break;
        }
        case MemoryType::Shared:
        {
            desc.resourceOptions = MTLResourceStorageModeShared;
            break;
        }
        case MemoryType::CPU:
        {
            desc.resourceOptions = MTLResourceStorageModeShared;
            break;
        }
        default:
        {
            GLEAM_ASSERT(false, "Metal: Unknown memory type given!");
            break;
        }
    }
    mHandle = [MetalDevice::GetHandle() newHeapWithDescriptor:desc];
}

Heap::~Heap()
{
    mHandle = nil;
}

#endif
