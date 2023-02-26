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

    size_t GetSize() const;

    BufferUsage GetUsage() const;

protected:

	void Allocate();

    void Free();

    const BufferUsage mUsage = BufferUsage::StorageBuffer;
    const MemoryType mMemoryType = MemoryType::Static;

    size_t mSize = 0;

    void* mContents = nullptr;
    NativeGraphicsHandle mMemory = nullptr;

};

} // namespace Gleam
