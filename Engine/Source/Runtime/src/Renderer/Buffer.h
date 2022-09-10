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

template<typename T, BufferUsage usage, MemoryType memoryType = MemoryType::Static>
class Buffer : public IBuffer
{
public:

    Buffer(size_t count)
		: IBuffer(count * sizeof(T), usage, memoryType)
	{

	}
    
    Buffer(const std::vector<T>& container)
        : IBuffer(container.data(), sizeof(T) * container.size(), usage, memoryType)
    {
        
    }
    
    template<size_t size>
    Buffer(const std::array<T, size>& container)
        : IBuffer(container.data(), sizeof(T) * container.size(), usage, memoryType)
    {
        
    }

	virtual ~Buffer() = default;

    void Resize(size_t count, bool keepContent = false)
    {
        IBuffer::Resize(sizeof(T) * count, keepContent);
    }
    
    void SetData(const T& t, size_t offset = 0) const
    {
        IBuffer::SetData(&t, offset, sizeof(T));
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

template<IndexType indexType, MemoryType memoryType = MemoryType::Static, typename T = typename std::conditional<indexType == IndexType::UINT32, uint32_t, uint16_t>::type>
using IndexBuffer = Buffer<T, BufferUsage::IndexBuffer, memoryType>;

template<typename T, MemoryType memoryType = MemoryType::Static>
using StorageBuffer = Buffer<T, BufferUsage::StorageBuffer, memoryType>;
    
template<typename T, MemoryType memoryType = MemoryType::Static>
using UniformBuffer = Buffer<T, BufferUsage::UniformBuffer, memoryType>;

template<typename T, MemoryType memoryType = MemoryType::Static>
using VertexBuffer = StorageBuffer<T, memoryType>;

} // namespace Gleam
