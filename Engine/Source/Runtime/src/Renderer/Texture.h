#pragma once
#include "TextureFormat.h"

namespace Gleam {

enum class FilterMode
{
	Point,
	Bilinear,
	Trilinear
};

enum class WrapMode
{
	Repeat,
	Clamp,
	Mirror,
	MirrorOnce
};

class Texture2D
{
public:

	Texture2D(uint32_t width, uint32_t height, bool useMipMap = false);
	Texture2D(uint32_t width, uint32_t height, TextureFormat format, bool useMipMap = false);

private:

	TextureFormat mFormat = TextureFormat::R8G8B8A8_SRGB;
	FilterMode mFilterMode = FilterMode::Point;
	WrapMode mWrapMode = WrapMode::Clamp;
	uint32_t mWidth = 0, mHeight = 0;
	uint32_t mAnistropicFilterLevel;
	uint32_t mMipMapCount = 0;
	float mMipMapBias;

	NativeGraphicsHandle mHandle;
	
};

} // namespace Gleam