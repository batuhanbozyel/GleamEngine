#include "gpch.h"
#include "Buffer.h"
#include "CommandBuffer.h"

using namespace Gleam;

Buffer::Buffer(const void* data, const BufferDescriptor& descriptor)
    : Buffer(descriptor)
{
    SetData(data, descriptor.size);
}

void Buffer::SetData(const void* data, size_t size, size_t offset) const
{
    if (mDescriptor.memoryType == MemoryType::Static)
    {
        BufferDescriptor descriptor;
        descriptor.size = size;
        descriptor.usage = BufferUsage::StagingBuffer;
        descriptor.memoryType = MemoryType::Stream;
        Buffer stagingBuffer(data, descriptor);
        
        CommandBuffer commandBuffer;
        commandBuffer.Begin();
        commandBuffer.CopyBuffer(stagingBuffer.GetHandle(), GetHandle(), size, 0, static_cast<uint32_t>(offset));
        commandBuffer.End();
        commandBuffer.Commit();
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
