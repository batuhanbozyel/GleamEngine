#pragma once
#include "GraphicsObject.h"
#include "IndexType.h"

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

/************************************************************************/
/* Buffer                                                               */
/************************************************************************/
class IBuffer : public GraphicsObject
{
public:

    IBuffer(size_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static);

    IBuffer(const void* data, size_t size, BufferUsage usage, MemoryType memoryType = MemoryType::Static);

    virtual ~IBuffer();

	static void Allocate(IBuffer& buffer);

	static void Free(IBuffer& buffer);

	static void Copy(const IBuffer& src, const IBuffer& dst, size_t srcOffset = 0, size_t dstOffset = 0);

    void Resize(size_t size, bool keepContent = false);

    void SetData(const void* data, size_t offset, size_t size) const;

    size_t GetSize() const;

    BufferUsage GetUsage() const;

protected:

	const BufferUsage mUsage = BufferUsage::StorageBuffer;
	const MemoryType mMemoryType = MemoryType::Static;

    size_t mSize = 0;
    
	void* mContents = nullptr;
	NativeGraphicsHandle mMemory = nullptr;

};

template<typename T, BufferUsage usage>
class Buffer : public IBuffer
{
public:

    Buffer(size_t count, MemoryType memoryType = MemoryType::Static)
		: IBuffer(count * sizeof(T), usage, memoryType)
	{

	}
    
    template<size_t size = 0>
    Buffer(const TArray<T, size>& container, MemoryType memoryType = MemoryType::Static)
        : IBuffer(container.data(), sizeof(T) * container.size(), usage, memoryType)
    {
        
    }

	virtual ~Buffer() = default;

    void Resize(size_t count, bool keepContent = false)
    {
        IBuffer::Resize(sizeof(T) * count, keepContent);
    }
    
    template<size_t size>
    void SetData(const std::array<T, size>& container, size_t offset = 0) const
    {
        IBuffer::SetData(container.data(), offset, sizeof(T) * container.size());
    }
    
    void SetData(const std::vector<T>& container, size_t offset = 0) const
    {
        IBuffer::SetData(container.data(), offset, sizeof(T) * container.size());
    }
    
	size_t GetCount() const
	{
		return mSize / sizeof(T);
	}

};

template<IndexType indexType, typename T = typename std::conditional<indexType == IndexType::UINT32, uint32_t, uint16_t>::type>
using IndexBuffer = Buffer<T, BufferUsage::IndexBuffer>;

template<typename T>
using StorageBuffer = Buffer<T, BufferUsage::StorageBuffer>;
    
template<typename T>
using UniformBuffer = Buffer<T, BufferUsage::UniformBuffer>;

template<typename T>
using VertexBuffer = StorageBuffer<T>;

} // namespace Gleam
