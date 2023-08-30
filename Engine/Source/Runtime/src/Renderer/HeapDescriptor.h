#pragma once

namespace Gleam {

enum class MemoryType
{
    GPU,
    Shared,
    CPU,
    Transient
};

struct HeapDescriptor
{
	MemoryType memoryType = MemoryType::GPU;
	size_t size = 0;
};

} // namespace Gleam
