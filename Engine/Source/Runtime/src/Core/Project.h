#pragma once
#include "EngineConfig.h"
#include "World/WorldConfig.h"
#include "Assets/AssetReference.h"

namespace Gleam {

struct Project
{
	TString name;
    Version version;
	Filesystem::path path;
	WorldConfig worldConfig;
};

} // namespace Gleam
