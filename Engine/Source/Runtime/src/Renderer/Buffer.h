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

	Buffer(uint32_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static);

    Buffer(const void* data, uint32_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static);

	virtual ~Buffer();

	static void Allocate(Buffer& buffer);

	static void Free(const Buffer& buffer);

	static void Copy(const Buffer& src, const Buffer& dst);

	void Resize(uint32_t size, bool keepContent = false);

	void SetData(const void* data, uint32_t offset, uint32_t size) const;

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

	VertexBuffer(uint32_t count)
		: Buffer(count * sizeof(T), BufferUsage::StorageBuffer)
	{

	}

	template<size_t size = 0>
    VertexBuffer(const TArray<T, size>& vertices)
        : Buffer(vertices.data(), vertices.size() * sizeof(T), BufferUsage::StorageBuffer)
    {

    }

	~VertexBuffer() = default;

	template<size_t size = 0>
    void SetData(const TArray<T, size>& vertices, uint32_t offset = 0)
    {
        Buffer::SetData(vertices.data(), offset, sizeof(T) * vertices.size());
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

	IndexBuffer(uint32_t count, IndexType indexType = IndexType::UINT32)
		: mIndexType(indexType), Buffer(count * SizeOfIndexType(indexType), BufferUsage::IndexBuffer)
	{
        
	}

	IndexBuffer(const TArray<uint16_t>& indices)
		: mIndexType(IndexType::UINT16), Buffer(indices.data(), indices.size() * sizeof(uint16_t), BufferUsage::IndexBuffer)
	{

	}

    IndexBuffer(const TArray<uint32_t>& indices)
        : mIndexType(IndexType::UINT32), Buffer(indices.data(), indices.size() * sizeof(uint32_t), BufferUsage::IndexBuffer)
    {
        
    }

	~IndexBuffer() = default;

	template<size_t size = 0>
    void SetData(const TArray<uint16_t, size>& indices, uint32_t offset = 0)
    {
		GLEAM_ASSERT(sizeof(uint16_t) == SizeOfIndexType(mIndexType));
        Buffer::SetData(indices.data(), offset, indices.size() * sizeof(uint16_t));
    }

	template<size_t size = 0>
	void SetData(const TArray<uint32_t, size>& indices, uint32_t offset = 0)
	{
		GLEAM_ASSERT(sizeof(uint32_t) == SizeOfIndexType(mIndexType));
		Buffer::SetData(indices.data(), offset, indices.size() * sizeof(uint32_t));
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
