#pragma once
#include "Container/Array.h"

namespace Gleam {

struct MeshDrawRange
{
	uint32_t first = 0;
	uint32_t count = 0;
};

struct MeshDrawList
{
	TArray<uint32_t> draws;
	TArray<MeshDrawRange> batches;
};

} // namespace Gleam
