#pragma once
#include "GraphicsObject.h"
#include "HeapDescriptor.h"

namespace Gleam {

class Buffer;
struct BufferDescriptor;

class Heap : public GraphicsObject
{
public:
    
    Heap() = default;
    
    Heap(const HeapDescriptor& descriptor);
    
    Heap(const Heap& other)
        : GraphicsObject(other), mDescriptor(other.mDescriptor)
    {
        
    }
    
    Heap& operator=(const Heap& other)
    {
        GraphicsObject::operator=(other);
        mDescriptor = other.mDescriptor;
        return *this;
    }

    void Dispose();

	Buffer CreateBuffer(const BufferDescriptor& descriptor, size_t offset = 0) const;
    
    const HeapDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}
    
private:

    HeapDescriptor mDescriptor;
    
};

} // namespace Gleam
