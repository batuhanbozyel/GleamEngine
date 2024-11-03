#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct RawTexture
{
	Gleam::TString name;
	int width, height, channels;
	uint8_t* pixels;
};

class TextureSource : public AssetPackage
{
public:
	AssetPackageType(TextureSource);

	struct ImportSettings
	{
		
	};

	bool Import(const Gleam::Filesystem::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
