#pragma once
#include "GraphicsObject.h"
#include "HeapDescriptor.h"

namespace Gleam {

class Buffer;
struct BufferDescriptor;

class Heap : public GraphicsObject
{
public:

	GLEAM_NONCOPYABLE(Heap);
    
    Heap(const HeapDescriptor& descriptor);

    ~Heap();

	Buffer CreateBuffer(const BufferDescriptor& descriptor, size_t offset = 0) const;

	void DestroyBuffer(const Buffer& buffer) const;
    
    const HeapDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}
    
private:

    const HeapDescriptor mDescriptor;
    
};

} // namespace Gleam
