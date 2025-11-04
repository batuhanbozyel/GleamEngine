#pragma once
#include "Assets/AssetReference.h"

namespace Gleam {

GSTRUCT(WorldConfig, "82C01D6D-F96B-4F56-8079-BACF82C01D49", Serializable)
{
	GFIELD("F02CEA32-D5FF-4253-BC16-8F8340FCF4A4", Serializable)
	uint32_t startingWorldIndex = 0;

	GFIELD("C56CB0E8-8651-46C0-A19B-083B639B195C", Serializable)
	TArray<AssetReference> worlds;
};

} // namespace Gleam
