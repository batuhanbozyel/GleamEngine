#pragma once
#include "GraphicsObject.h"

namespace Gleam {

enum class BufferUsage
{
	VertexBuffer,
	IndexBuffer,
	StorageBuffer,
	UniformBuffer,
	StagingBuffer
};

enum class MemoryType
{
	Static,
	Dynamic,
	Stream
};

enum class IndexType
{
	UINT16,
	UINT32
};

/************************************************************************/
/* Buffer                                                               */
/************************************************************************/
class Buffer : public GraphicsObject
{
public:

    Buffer(size_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static)
        : mSize(size), mUsage(usage), mMemoryType(memoryType)
    {
        Allocate(*this);
    }

    Buffer(const void* data, size_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static)
        : Buffer(size, usage, memoryType)
    {
        SetData(data, 0, size);
    }

	virtual ~Buffer()
    {
        Free(*this);
    }

	static void Allocate(Buffer& buffer);

	static void Free(Buffer& buffer);

	static void Copy(const Buffer& src, const Buffer& dst, size_t srcOffset = 0, size_t dstOffset = 0);

	void Resize(size_t size, bool keepContent = false)
    {
        GLEAM_ASSERT(!((size < mSize) && keepContent), "Cannot resize a buffer to a smaller size whilst keeping contents!");
        
        if (keepContent)
        {
            if (mMemoryType == MemoryType::Static)
            {
                Buffer stagingBuffer(mSize, BufferUsage::StagingBuffer, MemoryType::Static);
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

	void SetData(const void* data, size_t offset, size_t size) const
    {
        GLEAM_ASSERT(size <= mSize, "Cannot send data to the buffer if size is larger than the buffer size!");
        
        if (mMemoryType == MemoryType::Static)
        {
            Buffer stagingBuffer(data, size, BufferUsage::StagingBuffer, MemoryType::Stream);
            Copy(stagingBuffer, *this, 0, offset);
        }
        else
        {
            memcpy(As<uint8_t*>(mContents) + offset, data, size);
        }
    }
    
    template<typename Container>
    void SetData(const Container& data, size_t offset = 0) const
    {
        SetData(data.data(), offset, sizeof(typename Container::value_type) * data.size());
    }

    size_t GetSize() const
	{
		return mSize;
	}

	BufferUsage GetUsage() const
	{
		return mUsage;
	}

protected:

	BufferUsage mUsage = BufferUsage::StorageBuffer;
	MemoryType mMemoryType = MemoryType::Static;

	void* mContents = nullptr;
	NativeGraphicsHandle mMemory = nullptr;

    size_t mSize = 0;

};

/************************************************************************/
/* VertexBuffer                                                         */
/************************************************************************/
template<typename T>
class VertexBuffer final : public Buffer
{
public:

    VertexBuffer(uint32_t count, MemoryType memoryType = MemoryType::Static)
		: Buffer(count * sizeof(T), BufferUsage::StorageBuffer, memoryType)
	{

	}
    
    template<size_t size = 0>
    VertexBuffer(const TArray<T, size>& vertices, MemoryType memoryType = MemoryType::Static)
        : Buffer(vertices.data(), vertices.size() * sizeof(T), BufferUsage::StorageBuffer, memoryType)
    {

    }

	~VertexBuffer() = default;

    void Resize(uint32_t count, bool keepContent = false)
    {
        Buffer::Resize(sizeof(T) * count, keepContent);
    }

	uint32_t GetCount() const
	{
		return mSize / sizeof(T);
	}

};

/************************************************************************/
/* IndexBuffer                                                          */
/************************************************************************/
class IndexBuffer final : public Buffer
{
public:

    IndexBuffer(uint32_t count, IndexType indexType = IndexType::UINT32, MemoryType memoryType = MemoryType::Static)
		: mIndexType(indexType), Buffer(count * SizeOfIndexType(indexType), BufferUsage::IndexBuffer, memoryType)
	{
        
	}

	IndexBuffer(const TArray<uint16_t>& indices, MemoryType memoryType = MemoryType::Static)
		: mIndexType(IndexType::UINT16), Buffer(indices.data(), static_cast<uint32_t>(indices.size() * sizeof(uint16_t)), BufferUsage::IndexBuffer, memoryType)
	{

	}

    IndexBuffer(const TArray<uint32_t>& indices, MemoryType memoryType = MemoryType::Static)
        : mIndexType(IndexType::UINT32), Buffer(indices.data(), static_cast<uint32_t>(indices.size() * sizeof(uint32_t)), BufferUsage::IndexBuffer, memoryType)
    {
        
    }

	~IndexBuffer() = default;

    void Resize(uint32_t count, bool keepContent = false)
    {
        Buffer::Resize(SizeOfIndexType(mIndexType) * count, keepContent);
    }

	uint32_t GetCount() const
	{
		return mSize / SizeOfIndexType(mIndexType);
	}

	IndexType GetIndexType() const
	{
		return mIndexType;
	}

private:

	static constexpr uint32_t SizeOfIndexType(IndexType indexType)
	{
		switch (indexType)
		{
			case IndexType::UINT16: return sizeof(uint16_t);
			case IndexType::UINT32: return sizeof(uint32_t);
			default: return 0;
		}
	}

	const IndexType mIndexType = IndexType::UINT32;

};

} // namespace Gleam
