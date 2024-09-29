#pragma once
#include "GraphicsObject.h"
#include "HeapDescriptor.h"

namespace Gleam {

class Buffer;
class GraphicsDevice;

struct BufferDescriptor;

class Heap final : public GraphicsObject
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

    Buffer CreateBuffer(const BufferDescriptor& descriptor) const;

    void Reset() const
    {
        mStackPtr = 0;
    }

	size_t GetAlignment() const
	{
		return mAlignment;
	}

    const HeapDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }

private:

    mutable size_t mStackPtr = 0;

    size_t mAlignment = 0;

	void* mContents = nullptr;

    HeapDescriptor mDescriptor;

    GraphicsDevice* mDevice = nullptr;

};

namespace Utils {

static constexpr uint64_t AlignUp(const size_t offset, const size_t alignment)
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

} // namespace Utils

} // namespace Gleam
