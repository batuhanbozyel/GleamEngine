#pragma once
#include "GraphicsObject.h"

namespace Gleam {

class Heap;
class GraphicsDevice;

class Buffer final : public ShaderResource
{
    friend class Heap;
    friend class GraphicsDevice;
    
public:

	Buffer() = default;
    
    Buffer(const Buffer& other) = default;
    
    Buffer& operator=(const Buffer& other) = default;

    Buffer(NativeGraphicsHandle handle, size_t size, void* contents = nullptr)
        : ShaderResource(handle), mSize(size), mContents(contents)
    {
        
    }
    
    void* GetContents() const
    {
        return mContents;
    }
    
	size_t GetSize() const
    {
        return mSize;
    }
    
private:
    
	void* mContents = nullptr;

	size_t mSize;
    
};

} // namespace Gleam
