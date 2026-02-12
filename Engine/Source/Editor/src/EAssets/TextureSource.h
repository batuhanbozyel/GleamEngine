#pragma once
#include "AssetPackage.h"
#include "Renderer/TextureFormat.h"

namespace GEditor {

enum class TextureColorSpace
{
	Linear,
	sRGB
};

struct RawTexture
{
	Gleam::TString name;
	Gleam::TextureFormat format;
	int width, height, channels;
	void* pixels;
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
