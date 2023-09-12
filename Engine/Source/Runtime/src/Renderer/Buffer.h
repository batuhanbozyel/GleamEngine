#pragma once
#include "GraphicsObject.h"
#include "BufferDescriptor.h"

namespace Gleam {

class Buffer : public GraphicsObject
{
public:

	Buffer() = default;

	Buffer(NativeGraphicsHandle handle, const BufferDescriptor& descriptor, void* contents = nullptr);

	Buffer(const Buffer&);

	Buffer& operator=(const Buffer& other);

	void SetData(const void* data, size_t size, size_t offset = 0) const;
    
    const BufferDescriptor& GetDescriptor() const;
    
private:
    
	void* mContents = nullptr;

    const BufferDescriptor mDescriptor;
    
};

} // namespace Gleam
