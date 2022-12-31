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

struct BufferInfo
{
	BufferUsage usage = BufferUsage::StorageBuffer;
	MemoryType memoryType = MemoryType::Static;
};

} // namespace Gleam