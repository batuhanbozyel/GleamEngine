#pragma once
#include "Heap.h"

namespace Gleam {

class Heap;
class GraphicsDevice;

struct BufferDescriptor
{
    TString name;
	MemoryType memoryType = MemoryType::GPU;
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

	Buffer(const BufferDescriptor& descriptor, NativeGraphicsHandle handle, ShaderResourceIndex view, void* contents, size_t alignment)
		: ShaderResource(handle, view)
		, mDescriptor(descriptor)
		, mAlignment(alignment)
		, mContents(contents)
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

	size_t GetAlignment() const
	{
		return mAlignment;
	}
    
    const BufferDescriptor& GetDescriptor() const
    {
        return mDescriptor;
    }
    
private:

	size_t mAlignment = 0;
    
	void* mContents = nullptr;

    BufferDescriptor mDescriptor;
    
};

} // namespace Gleam
