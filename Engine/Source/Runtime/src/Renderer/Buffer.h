#pragma once
#include "GraphicsObject.h"
#include "BufferDescriptor.h"

namespace Gleam {

class Buffer : public GraphicsObject
{
public:

	Buffer() = default;

	Buffer(NativeGraphicsHandle handle, const BufferDescriptor& descriptor, void* contents = nullptr);

	Buffer(const Buffer& other) = default;

	Buffer& operator=(const Buffer& other) = default;
    
    void Dispose();

	void SetData(const void* data, size_t size, size_t offset = 0) const;
    
    void* GetContents() const;
    
    const BufferDescriptor& GetDescriptor() const;
    
private:
    
	void* mContents = nullptr;

    BufferDescriptor mDescriptor;
    
};

} // namespace Gleam
