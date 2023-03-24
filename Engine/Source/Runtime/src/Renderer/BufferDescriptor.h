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

enum class MemoryType
{
    Static,
    Dynamic,
    Stream
};

struct BufferDescriptor
{
	BufferUsage usage = BufferUsage::StorageBuffer;
	MemoryType memoryType = MemoryType::Static;
	size_t size = 0;
};

} // namespace Gleam
