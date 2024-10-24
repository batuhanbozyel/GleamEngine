#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor) const
{
    auto alignedStackPtr = Utils::AlignUp(mStackPtr, mAlignment);
    auto newStackPtr = alignedStackPtr + descriptor.size;

    if (Utils::AlignUp(mDescriptor.size, mAlignment) < newStackPtr)
    {
        GLEAM_ASSERT(false, "Metal: Heap is full!");
        return Buffer(descriptor);
    }
    mStackPtr = newStackPtr;

    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> mtlBuffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions offset:alignedStackPtr];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        contents = [mtlBuffer contents];
    }
    
    TStringStream resourceName;
    resourceName << mDescriptor.name << "::" << descriptor.name;
    [mtlBuffer setLabel:TO_NSSTRING(resourceName.str().c_str())];
    
    Buffer buffer(descriptor);
    buffer.mHandle = mtlBuffer;
    buffer.mContents = contents;
    buffer.mResourceView = mDescriptor.memoryType == MemoryType::CPU ? InvalidResourceIndex : static_cast<MetalDevice*>(mDevice)->CreateResourceView(buffer);
    return buffer;
}

#endif
