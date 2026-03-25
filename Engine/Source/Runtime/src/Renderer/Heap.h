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

	size_t GetAlignment() const
	{
		return mAlignment;
	}

    const HeapDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }

private:

    size_t mAlignment = 0;

    HeapDescriptor mDescriptor;

};

} // namespace Gleam
