#pragma once
#include "EngineConfig.h"
#include "World/WorldConfig.h"

namespace Gleam {

struct Project
{
	TString name;
    Version version;
	Filesystem::Path path;
	WorldConfig worldConfig;
};

} // namespace Gleam

GLEAM_TYPE(Gleam::Project, Guid("13DAA9F7-712B-4D48-AB57-4F134F596D41"))
    GLEAM_FIELD(name, Serializable())
    GLEAM_FIELD(version, Serializable())
    GLEAM_FIELD(path, Serializable())
    GLEAM_FIELD(worldConfig, Serializable())
GLEAM_END
