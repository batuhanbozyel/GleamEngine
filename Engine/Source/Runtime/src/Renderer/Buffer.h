#pragma once

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

	Buffer(uint32_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static)
        : mSize(size), mUsage(usage), mMemoryType(memoryType)
    {
        Allocate(*this);
    }

    Buffer(const void* data, uint32_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static)
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

	static void Copy(const Buffer& src, const Buffer& dst, uint32_t srcOffset = 0, uint32_t dstOffset = 0);

	void Resize(uint32_t size, bool keepContent = false)
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

	void SetData(const void* data, uint32_t offset, uint32_t size) const
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

	uint32_t GetSize() const
	{
		return mSize;
	}

	BufferUsage GetUsage() const
	{
		return mUsage;
	}

protected:

	BufferUsage mUsage;
	MemoryType mMemoryType;

	void* mContents;
	NativeGraphicsHandle mMemory;

	uint32_t mSize = 0;

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
        : Buffer(vertices.data(), static_cast<uint32_t>(vertices.size() * sizeof(T)), BufferUsage::StorageBuffer, memoryType)
    {

    }

	~VertexBuffer() = default;

    void Resize(uint32_t count, bool keepContent = false)
    {
        Buffer::Resize(sizeof(T) * count, keepContent);
    }
    
	template<size_t size = 0>
    void SetData(const TArray<T, size>& vertices, uint32_t offset = 0)
    {
        Buffer::SetData(vertices.data(), offset, static_cast<uint32_t>(sizeof(T) * vertices.size()));
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
    
	template<size_t size = 0>
    void SetData(const TArray<uint16_t, size>& indices, uint32_t offset = 0)
    {
		GLEAM_ASSERT(sizeof(uint16_t) == SizeOfIndexType(mIndexType));
        Buffer::SetData(indices.data(), offset, static_cast<uint32_t>(indices.size() * sizeof(uint16_t)));
    }

	template<size_t size = 0>
	void SetData(const TArray<uint32_t, size>& indices, uint32_t offset = 0)
	{
		GLEAM_ASSERT(sizeof(uint32_t) == SizeOfIndexType(mIndexType));
		Buffer::SetData(indices.data(), offset, static_cast<uint32_t>(indices.size() * sizeof(uint32_t)));
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

	const IndexType mIndexType;

};

} // namespace Gleam
