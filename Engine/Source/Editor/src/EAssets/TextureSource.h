#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct TextureSource : AssetPackage
{
	struct ImportSettings
	{
		
	};

	static TextureSource Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
