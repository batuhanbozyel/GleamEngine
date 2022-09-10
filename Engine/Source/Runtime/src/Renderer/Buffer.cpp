#include "gpch.h"
#include "Buffer.h"

using namespace Gleam;

IBuffer::IBuffer(size_t size, BufferUsage usage, MemoryType memoryType)
    : mSize(size), mUsage(usage), mMemoryType(memoryType)
{
    Allocate(*this);
}

IBuffer::IBuffer(const void* data, size_t size, BufferUsage usage, MemoryType memoryType)
    : IBuffer(size, usage, memoryType)
{
    SetData(data, 0, size);
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
            Copy(*this, stagingBuffer);
            Free(*this);

            mSize = size;
            Allocate(*this);

            Copy(stagingBuffer, *this);
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

void IBuffer::SetData(const void* data, size_t offset, size_t size) const
{
    GLEAM_ASSERT(size <= mSize, "Cannot send data to the buffer if size is larger than the buffer size!");
    
    if (mMemoryType == MemoryType::Static)
    {
        IBuffer stagingBuffer(data, size, BufferUsage::StagingBuffer, MemoryType::Stream);
        Copy(stagingBuffer, *this, 0, offset);
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
