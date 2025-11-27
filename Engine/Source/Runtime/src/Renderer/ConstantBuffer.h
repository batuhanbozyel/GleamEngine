#pragma once
#include "GraphicsObject.h"

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

	size_t Allocate(size_t size);

	NativeGraphicsHandle GetHandle() const
	{
		return mHandle;
	}

private:

	size_t mStackPtr = 0;

	size_t mCapacity = 0;

	size_t mAlignment = 4;

	void* mContents = nullptr;

	NativeGraphicsHandle mHandle;

};

} // namespace Gleam
