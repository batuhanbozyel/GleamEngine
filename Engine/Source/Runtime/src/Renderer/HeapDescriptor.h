#pragma once
#include "Container/String.h"

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
	MemoryType type;
};

namespace Utils {

static constexpr const char* MemoryTypeToString(MemoryType type)
{
	switch (type)
	{
		case MemoryType::CPU: return "CPU";
		case MemoryType::Shared: return "Shared";
		case MemoryType::GPU: return "GPU";
		case MemoryType::Transient: return "Transient";
		default: return "UNKNOWN";
	}
}

} // namespace Utils

} // namespace Gleam
