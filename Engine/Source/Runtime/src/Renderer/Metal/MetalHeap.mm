#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Heap::Heap(const HeapDescriptor& descriptor)
    : mDescriptor(descriptor)
{
    MTLHeapDescriptor* desc = [MTLHeapDescriptor new];
    desc.resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType);
    desc.size = descriptor.size;
    mHandle = [MetalDevice::GetHandle() newHeapWithDescriptor:desc];
}

Heap::~Heap()
{
    mHandle = nil;
}

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> buffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions offset:offset];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        contents = [buffer contents];
    }
    return Buffer(buffer, descriptor, contents);
}

void Heap::DestroyBuffer(const Buffer& buffer) const
{
    // does nothing on Metal
}

#endif
