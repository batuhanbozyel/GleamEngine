#include "gpch.h"
#include "Buffer.h"
#include "CommandBuffer.h"

using namespace Gleam;

Buffer::Buffer(const BufferDescriptor& descriptor)
    : mSize(descriptor.size), mUsage(descriptor.usage), mMemoryType(descriptor.memoryType)
{
    Allocate();
}

Buffer::Buffer(const void* data, const BufferDescriptor& descriptor)
    : Buffer(descriptor)
{
    SetData(data, descriptor.size);
}

Buffer::~Buffer()
{
    Free();
}

void Buffer::SetData(const void* data, size_t size, size_t offset = 0) const
{
	if (mMemoryType == MemoryType::Static)
    {
        Buffer stagingBuffer(data, size, BufferUsage::StagingBuffer, MemoryType::Stream);
        
        CommandBuffer commandBuffer;
        commandBuffer.Begin();
        commandBuffer.CopyBuffer(stagingBuffer, *this, size, 0, static_cast<uint32_t>(offset));
        commandBuffer.End();
        commandBuffer.Commit();
    }
    else
    {
        memcpy(As<uint8_t*>(mContents) + offset, data, size);
    }
}

size_t Buffer::GetSize() const
{
    return mSize;
}

BufferUsage Buffer::GetUsage() const
{
    return mUsage;
}
