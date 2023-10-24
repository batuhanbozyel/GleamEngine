#pragma once
#include "GraphicsObject.h"
#include "BufferDescriptor.h"

namespace Gleam {

class GraphicsDevice;

class Buffer final : public GraphicsObject
{
    friend class GraphicsDevice;
    
public:

	Buffer() = default;
    
    Buffer(const Buffer& other) = default;
    
    Buffer& operator=(const Buffer& other) = default;

    Buffer(NativeGraphicsHandle handle, const BufferDescriptor& descriptor, void* contents = nullptr)
        : GraphicsObject(handle), mDescriptor(descriptor), mContents(contents)
    {
        
    }
    
    void* GetContents() const
    {
        return mContents;
    }
    
    const BufferDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }
    
private:
    
	void* mContents = nullptr;

    BufferDescriptor mDescriptor;
    
};

} // namespace Gleam
