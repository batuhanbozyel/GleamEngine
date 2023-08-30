#pragma once
#include "GraphicsObject.h"
#include "HeapDescriptor.h"

namespace Gleam {

class Heap : public GraphicsObject
{
public:
    
	Heap(const HeapDescriptor& descriptor);
    
    ~Heap();

private:

	HeapDescriptor mDescriptor;

};

} // namespace Gleam
