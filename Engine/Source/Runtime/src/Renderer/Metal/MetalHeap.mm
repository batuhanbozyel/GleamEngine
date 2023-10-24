#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
    auto alignedStackPtr = Utils::AlignTo(mStackPtr, mAlignment);
    auto newStackPtr = alignedStackPtr + descriptor.size;

    if (mDescriptor.size < newStackPtr)
    {
        GLEAM_ASSERT(false, "Metal: Heap is full!");
        return Buffer(nil, descriptor, nullptr);
    }
    mStackPtr = newStackPtr;

    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> buffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions offset:alignedStackPtr];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
            contents = [buffer contents];
    }
    return Buffer(buffer, descriptor, contents);
}

#endif
