#pragma once
#include "Core/Attributes.h"
#include "Container/Array.h"
#include "Container/String.h"
#include "IO/Path.h"

namespace Gleam {

GSTRUCT(RecentProject, "1AA8ADB5-313A-4928-99F2-BE7A2FC6BB24", Serializable)
{
	GFIELD("11B9698A-55D1-4486-85E9-36F16C651520", Serializable)
	Path path;

	GFIELD("4D5A27CB-7EB4-4B6B-B0B1-99BF188CC9E0", Serializable)
	TString name;

	GFIELD("B70BC483-689E-4DB9-A7E0-3055C2AAE43B", Serializable)
	uint64_t lastOpened = 0;
};

GSTRUCT(LauncherState, "FB26FD6A-997D-403A-88A5-B0B724AA936F", Serializable)
{
	GFIELD("88507139-CF5A-49AC-86F1-2E7412B3F97A", Serializable)
	TArray<RecentProject> recentProjects;
};

} // namespace Gleam
