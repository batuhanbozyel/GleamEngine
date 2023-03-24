#pragma once
#include "GraphicsObject.h"
#include "BufferDescriptor.h"

namespace Gleam {

class Buffer : public GraphicsObject
{
public:
    
    Buffer(const BufferDescriptor& descriptor);
    
    Buffer(const void* data, const BufferDescriptor& descriptor);
    
    virtual ~Buffer();
    
    void SetData(const void* data, size_t size, size_t offset = 0) const;
    
    const BufferDescriptor& GetDescriptor() const;
    
protected:
    
    void* mContents = nullptr;
    
    const BufferDescriptor mDescriptor;
    
};

} // namespace Gleam
