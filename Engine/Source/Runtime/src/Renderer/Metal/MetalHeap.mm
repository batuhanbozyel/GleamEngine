#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "MetalUtils.h"

using namespace Gleam;

static uint64_t AlignTo(const uint64_t offset, const uint64_t alignment)
{
    const bool isAligned = offset % alignment == 0;
    return isAligned ? offset : ((offset / alignment) + 1) * alignment;
}

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
    MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(mDescriptor.memoryType);
    MTLSizeAndAlign sizeAndAlign = [mDevice->GetHandle() heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];
    
    id<MTLHeap> heap = mHandle;
    id<MTLBuffer> buffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions offset:AlignTo(offset, sizeAndAlign.align)];

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
    {
        contents = [buffer contents];
    }
    return Buffer(buffer, descriptor, contents);
}

#endif
