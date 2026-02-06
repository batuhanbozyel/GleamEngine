#pragma once
#include "AssetPackage.h"

namespace GEditor {

struct RawTexture
{
	Gleam::TString name;
	int width, height, channels;
	void* pixels;
};

enum class TextureColorSpace
{
	Linear,
	sRGB
};

class TextureSource : public AssetPackage
{
public:
	AssetPackageType(TextureSource);

	struct ImportSettings
	{
		TextureColorSpace colorSpace = TextureColorSpace::Linear;
		bool generateMips = false;
		bool hdr = false;
	};

	bool Import(const Gleam::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
