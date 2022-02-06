#pragma once
#include "Texture.h"

namespace Gleam {

class TextureLibrary
{
public:

	static void Init();

	static void Destroy();

	static Ref<Texture2D> CreateTexture2D(const TextureFormat& format, bool multisample = false);

	static void ClearCache();

private:

	static Ref<Texture2D> mTexture2DCache;

};

} // namespace Gleam