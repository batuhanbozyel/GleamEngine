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
};

} // namespace Gleam
