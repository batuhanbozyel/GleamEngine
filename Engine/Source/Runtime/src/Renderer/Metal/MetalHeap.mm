#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Heap::Heap(const HeapDescriptor& descriptor)
    : mDescriptor(descriptor)
{
    MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType);
    MTLSizeAndAlign sizeAndAlign = [MetalDevice::GetHandle() heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];
    
    MTLHeapDescriptor* desc = [MTLHeapDescriptor new];
    desc.resourceOptions = resourceOptions;
    desc.size = sizeAndAlign.size;
    mHandle = [MetalDevice::GetHandle() newHeapWithDescriptor:desc];
}

void Heap::Dispose()
{
    mHandle = nil;
}

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> buffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        contents = [buffer contents];
    }
    return Buffer(buffer, descriptor, contents);
}

void Buffer::Dispose()
{
    mHandle = nil;
}

#endif
