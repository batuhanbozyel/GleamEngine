#pragma once
#include "Heap.h"
#include "Buffer.h"

namespace Gleam {

class GraphicsDevice;

class ConstantBuffer
{
public:

	ConstantBuffer(GraphicsDevice* device, size_t size);

	~ConstantBuffer();

	void Reset();

	template<typename T>
	size_t Write(const T& data)
	{
		return Write(&data, sizeof(T));
	}

	size_t Write(const void* data, size_t size);

	NativeGraphicsHandle GetHandle() const
	{
		return mBuffer.GetHandle();
	}

private:

	size_t mStackPtr = 0;

	Heap mHeap;

	Buffer mBuffer;

	GraphicsDevice* mDevice;

};

} // namespace Gleam
