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

	Buffer CreateBuffer(const BufferDescriptor& descriptor, size_t offset = 0) const;
    
    const HeapDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}
    
private:

    HeapDescriptor mDescriptor;
    
    const GraphicsDevice* mDevice = nullptr;
    
};

} // namespace Gleam
