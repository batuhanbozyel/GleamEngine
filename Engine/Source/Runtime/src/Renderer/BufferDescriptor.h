#pragma once

namespace Gleam {

enum class BufferUsage
{
    VertexBuffer,
    IndexBuffer,
    StorageBuffer,
    UniformBuffer,
    StagingBuffer
};

struct BufferDescriptor
{
	BufferUsage usage = BufferUsage::StorageBuffer;
    size_t size = 0;
    
    BufferDescriptor& operator=(const BufferDescriptor& other)
    {
        usage = other.usage;
        size = other.size;
        return *this;
    }
};

} // namespace Gleam
