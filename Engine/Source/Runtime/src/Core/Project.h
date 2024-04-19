#pragma once
#include "EngineConfig.h"
#include "World/WorldConfig.h"
#include "Assets/AssetReference.h"

namespace Gleam {

struct Project
{
	EngineConfig engineConfig;
	WorldConfig worldConfig;
	Filesystem::path path;
	TString name;
};

} // namespace Gleam
