#pragma once
#include "../Buffer.h"

namespace Gleam {

class StackAllocator final
{
public:

	Buffer CreateBuffer(const BufferDescriptor& descriptor);

private:
    
	size_t mOffset = 0;

};

} // namespace Gleam
