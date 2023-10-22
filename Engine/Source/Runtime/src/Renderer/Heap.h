#pragma once
#include "GraphicsObject.h"
#include "HeapDescriptor.h"

namespace Gleam {

class Buffer;
class GraphicsDevice;
struct BufferDescriptor;

class Heap : public GraphicsObject
{
	friend class GraphicsDevice;

public:

	Heap() = default;

	Heap(const Heap& other) = default;

	Heap& operator=(const Heap&) = default;

	Heap(const HeapDescriptor& descriptor)
		: mDescriptor(descriptor)
	{

	}

	Buffer CreateBuffer(const BufferDescriptor& descriptor);

	void Reset()
	{
		mStackPtr = 0;
	}

	const HeapDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

private:

	size_t mStackPtr = 0;

	size_t mAlignment = 0;

	HeapDescriptor mDescriptor;

	const GraphicsDevice* mDevice = nullptr;

};

namespace Utils {

static constexpr uint64_t AlignTo(const size_t offset, const size_t alignment)
{
	const bool isAligned = offset % alignment == 0;
	return isAligned ? offset : ((offset / alignment) + 1) * alignment;
}

} // namespace Utils

} // namespace Gleam
