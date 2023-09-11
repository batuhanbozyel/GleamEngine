#include "gpch.h"
#include "Heap.h"
#include "Buffer.h"
#include "CommandBuffer.h"

using namespace Gleam;

Buffer::Buffer(NativeGraphicsHandle handle, const BufferDescriptor& descriptor, void* contents)
    : mHandle(handle), mDescriptor(descriptor), mContents(contents)
{
    
}

void Buffer::SetData(const void* data, size_t size, size_t offset) const
{
    if (mContents == nullptr)
    {
		HeapDescriptor heapDesc;
		heapDesc.size = size;
        heapDesc.memoryType = MemoryType::CPU;
		Heap heap(heapDesc);

        BufferDescriptor bufferDesc;
        bufferDesc.size = size;
        bufferDesc.usage = BufferUsage::StagingBuffer;
		Buffer stagingBuffer = heap.CreateBuffer(bufferDesc);
		stagingBuffer.SetData(data, size);
        
        CommandBuffer commandBuffer;
        commandBuffer.Begin();
        commandBuffer.CopyBuffer(stagingBuffer.GetHandle(), GetHandle(), size, 0, static_cast<uint32_t>(offset));
        commandBuffer.End();
        commandBuffer.Commit();

		heap.DestroyBuffer(stagingBuffer);
    }
    else
    {
        memcpy(As<uint8_t*>(mContents) + offset, data, size);
    }
}

const BufferDescriptor& Buffer::GetDescriptor() const
{
    return mDescriptor;
}
