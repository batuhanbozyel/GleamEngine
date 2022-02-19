#pragma once

namespace Gleam {

enum class BufferUsage
{
	VertexBuffer,
	IndexBuffer,
	StorageBuffer,
	UniformBuffer
};

class Buffer
{
public:

	Buffer(uint32_t size, BufferUsage usage);
	~Buffer();

	void SetData(const void* data, uint32_t offset, uint32_t size) const;

	NativeGraphicsHandle GetNativeHandle() const
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

} // namespace Gleam