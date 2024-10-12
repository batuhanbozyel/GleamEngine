#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

class MaterialSource : public AssetPackage
{
public:
	AssetPackageType(MaterialSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
