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
    TString name;
	MemoryType memoryType = MemoryType::GPU;
	size_t size = 0;
    
    bool operator==(const HeapDescriptor& other) const
    {
        return memoryType == other.memoryType && size == other.size;
    }
};

struct MemoryRequirements
{
	size_t size;
	size_t alignment;
};

} // namespace Gleam
