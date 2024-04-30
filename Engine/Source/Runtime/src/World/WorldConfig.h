#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

struct WorldConfig
{
	uint32_t startingWorldIndex = 0;
	TArray<AssetReference> worlds;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::WorldConfig, Guid("82C01D6D-F96B-4F56-8079-BACF82C01D49"))
    GLEAM_FIELD(startingWorldIndex, Serializable())
    GLEAM_FIELD(worlds, Serializable())
GLEAM_END
