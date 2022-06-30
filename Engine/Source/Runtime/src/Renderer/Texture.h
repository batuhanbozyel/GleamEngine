#pragma once
#include "TextureFormat.h"

namespace Gleam {

class Texture2D final : public GraphicsObject
{
public:

	Texture2D(uint32_t width, uint32_t height, TextureFormat format, bool useMipMap = false);
	~Texture2D();

	void SetPixels(const TArray<uint8_t>& pixels) const;

private:

	static constexpr uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
	{
		return Math::Floor(Math::Log2(Math::Max(width, height))) + 1;
	}

	TextureFormat mFormat = TextureFormat::R8G8B8A8_SRGB;
	FilterMode mFilterMode = FilterMode::Point;
	WrapMode mWrapMode = WrapMode::Clamp;
	uint32_t mWidth = 0, mHeight = 0;
	uint32_t mMipMapCount = 0;

	uint32_t mSize = 0;
	NativeGraphicsHandle mMemory;
	NativeGraphicsHandle mImageView;
	
};

class RenderTexture final : public GraphicsObject
{
public:


private:



};

} // namespace Gleam
