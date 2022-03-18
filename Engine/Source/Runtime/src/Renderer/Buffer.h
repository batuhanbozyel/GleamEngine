#pragma once

namespace Gleam {

enum class BufferUsage
{
	VertexBuffer,
	IndexBuffer,
	StorageBuffer,
	UniformBuffer
};

enum class IndexType
{
	UINT16,
	UINT32
};

/************************************************************************/
/* Buffer                                                               */
/************************************************************************/
class Buffer
{
public:

	Buffer(uint32_t size, BufferUsage usage);
	~Buffer();

	void SetData(const void* data, uint32_t offset, uint32_t size) const;

	NativeGraphicsHandle GetBuffer() const
	{
		return mBuffer;
	}

	uint32_t GetSize() const
	{
		return mSize;
	}

protected:

	void* mContents;
	NativeGraphicsHandle mMemory;
	NativeGraphicsHandle mBuffer;

	const uint32_t mSize = 0;

};

/************************************************************************/
/* VertexBuffer                                                         */
/************************************************************************/
template<typename T>
class VertexBuffer : public Buffer
{
public:

	VertexBuffer(uint32_t count)
		: mCount(count), Buffer(count * sizeof(T), BufferUsage::StorageBuffer)
	{

	}

	~VertexBuffer() = default;

	uint32_t GetCount() const
	{
		return mCount;
	}

private:

	uint32_t mCount;

};

/************************************************************************/
/* IndexBuffer                                                          */
/************************************************************************/
class IndexBuffer : public Buffer
{
public:

	IndexBuffer(uint32_t count, IndexType indexType = IndexType::UINT32)
		: mCount(count), mIndexType(indexType), Buffer(count* SizeOfIndexType(indexType), BufferUsage::IndexBuffer)
	{

	}

	~IndexBuffer() = default;

	uint32_t GetCount() const
	{
		return mCount;
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

	uint32_t mCount;
	IndexType mIndexType;

};

} // namespace Gleam