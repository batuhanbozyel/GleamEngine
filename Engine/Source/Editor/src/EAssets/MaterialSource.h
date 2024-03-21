#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct MaterialSource : AssetPackage
{
	struct ImportSettings
	{
		
	};

	static MaterialSource Import(const Gleam::Filesystem::path& path, const ImportSettings& settings);
};

} // namespace GEditor