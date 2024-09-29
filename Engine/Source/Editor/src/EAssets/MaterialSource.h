#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct MaterialSource : AssetPackage
{
	AssetPackageType(MaterialSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
