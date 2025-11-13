#pragma once
#include "EngineConfig.h"
#include "World/WorldConfig.h"
#include "Container/String.h"
#include "IO/Filesystem.h"

namespace Gleam {

GSTRUCT(Project, "13DAA9F7-712B-4D48-AB57-4F134F596D41", Serializable)
{
	GFIELD("5DECFB5F-1155-447B-A5E6-583A1E27D631", Serializable)
	TString name;

	GFIELD("D233B73D-A86B-468B-9011-B7C47B5C8E73", Serializable)
    Version version;

	GFIELD("AD817DA9-4743-46D5-86A2-8F36FC82BBA6", Serializable)
	WorldConfig worldConfig;

	Path path = Filesystem::WorkingDirectory();
};

} // namespace Gleam
