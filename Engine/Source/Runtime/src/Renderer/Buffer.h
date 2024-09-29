#pragma once
#include "GraphicsObject.h"

namespace Gleam {

class Heap;
class GraphicsDevice;

struct BufferDescriptor
{
    TString name;
    size_t size = 0;
    
    bool operator==(const BufferDescriptor& other) const
    {
        return size == other.size;
    }
};

class Buffer final : public ShaderResource
{
    friend class Heap;
    friend class GraphicsDevice;
    
public:

	Buffer() = default;
    
    Buffer(const Buffer& other) = default;
    
    Buffer& operator=(const Buffer& other) = default;

    Buffer(const BufferDescriptor& descriptor)
        : mDescriptor(descriptor)
    {
        
    }
    
    void* GetContents() const
    {
        return mContents;
    }
    
    size_t GetSize() const
    {
        return mDescriptor.size;
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
