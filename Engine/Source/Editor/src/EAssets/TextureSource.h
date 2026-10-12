#pragma once
#include "AssetPackage.h"
#include "Renderer/TextureFormat.h"
#include "Core/Attributes.h"

namespace GEditor {

GENUM(TextureColorSpace, "CAF0CB90-133F-4055-ACC0-22B2F11D79E5", PrettyName("Color Space"))
{
	GITEM(Linear, "2E59D6A1-097B-4CE0-A768-A7303F02FC45", PrettyName("Linear")),
	GITEM(sRGB, "3FFCA7D4-D5EC-4446-8CB4-1802DDBAF95E", PrettyName("sRGB"))
};

GSTRUCT(TextureImportSettings, "0178B235-E69A-4E6C-BCB4-1739D64BA7C0", Serializable, PrettyName("Texture"))
{
	GFIELD("660B640F-55A3-4DEC-960C-411746C43847", Serializable, PrettyName("Color Space"))
	TextureColorSpace colorSpace = TextureColorSpace::Linear;

	GFIELD("1D35614C-493C-434E-94B7-929D65962411", Serializable, PrettyName("Generate Mipmaps"))
	bool generateMips = false;
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

	using ImportSettings = TextureImportSettings;

	bool Import(const Gleam::Path& path, const ImportSettings& settings);
};

} // namespace GEditor
