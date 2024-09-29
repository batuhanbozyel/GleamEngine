#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct TextureSource : AssetPackage
{
	AssetPackageType(TextureSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
