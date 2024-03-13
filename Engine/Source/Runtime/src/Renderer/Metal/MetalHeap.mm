#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(size_t size) const
{
    auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
    auto newStackPtr = alignedStackPtr + size;

    if (Utils::AlignUp(mDescriptor.size, mAlignment) < newStackPtr)
    {
        GLEAM_ASSERT(false, "Metal: Heap is full!");
        return Buffer(nil, size, nullptr);
    }
    mStackPtr = newStackPtr;

    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> mtlBuffer = [heap newBufferWithLength:size options:heap.resourceOptions offset:alignedStackPtr];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        contents = [mtlBuffer contents];
    }
    Buffer buffer(mtlBuffer, size, contents);
    buffer.mResourceView = mDescriptor.memoryType == MemoryType::CPU ? InvalidResourceIndex : static_cast<MetalDevice*>(mDevice)->CreateResourceView(buffer);
    return buffer;
}

#endif
