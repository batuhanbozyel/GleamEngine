#include "gpch.h"
#include "Buffer.h"
#include "CommandBuffer.h"

using namespace Gleam;

IBuffer::IBuffer(size_t size, BufferUsage usage, MemoryType memoryType)
    : mSize(size), mUsage(usage), mMemoryType(memoryType)
{
    Allocate(*this);
}

IBuffer::IBuffer(const void* data, size_t size, BufferUsage usage, MemoryType memoryType)
    : IBuffer(size, usage, memoryType)
{
    SetData(data, size);
}

IBuffer::~IBuffer()
{
    Free(*this);
}

void IBuffer::Resize(size_t size, bool keepContent)
{
    GLEAM_ASSERT(!((size < mSize) && keepContent), "Cannot resize a buffer to a smaller size whilst keeping contents!");
    
    if (keepContent)
    {
        if (mMemoryType == MemoryType::Static)
        {
            IBuffer stagingBuffer(mSize, BufferUsage::StagingBuffer, MemoryType::Static);
            
            CommandBuffer commandBuffer;
            commandBuffer.Begin();
            commandBuffer.CopyBuffer(*this, stagingBuffer, mSize);
            
            Free(*this);
            mSize = size;
            Allocate(*this);
            
            commandBuffer.CopyBuffer(stagingBuffer, *this, mSize);
            commandBuffer.End();
            commandBuffer.Commit();
        }
        else
        {
            TArray<uint8_t> stagingBuffer(mSize);
            memcpy(stagingBuffer.data(), mContents, mSize);
            Free(*this);

            mSize = size;
            Allocate(*this);

            memcpy(mContents, stagingBuffer.data(), stagingBuffer.size());
        }
    }
    else
    {
        mSize = size;
        Free(*this);
        Allocate(*this);
    }
}

void IBuffer::SetData(const void* data, size_t size, size_t offset) const
{
    GLEAM_ASSERT(size <= mSize, "Cannot send data to the buffer if size is larger than the buffer size!");
    
    if (mMemoryType == MemoryType::Static)
    {
        IBuffer stagingBuffer(data, size, BufferUsage::StagingBuffer, MemoryType::Stream);
        
        CommandBuffer commandBuffer;
        commandBuffer.Begin();
        commandBuffer.CopyBuffer(stagingBuffer, *this, size, 0, offset);
        commandBuffer.End();
        commandBuffer.Commit();
    }
    else
    {
        memcpy(As<uint8_t*>(mContents) + offset, data, size);
    }
}

size_t IBuffer::GetSize() const
{
    return mSize;
}

BufferUsage IBuffer::GetUsage() const
{
    return mUsage;
}
