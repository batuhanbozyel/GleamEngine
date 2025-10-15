#pragma once
#include "AssetPackage.h"

namespace GEditor {

class MaterialSource : public AssetPackage
{
public:
	AssetPackageType(MaterialSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
