#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

struct WorldConfig
{
	size_t startingWorldIndex = 0;
	TArray<AssetReference> worlds;
};

} // namespace Gleam
