#pragma once
#include "HeapDescriptor.h"

namespace Gleam {

enum class BufferUsage
{
    VertexBuffer,
    IndexBuffer,
    StorageBuffer,
    UniformBuffer,
    StagingBuffer
};

struct BufferDescriptor : public HeapDescriptor
{
	BufferUsage usage = BufferUsage::StorageBuffer;
};

} // namespace Gleam
