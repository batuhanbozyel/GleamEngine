#pragma once
#include "Texture.h"

namespace Gleam {

class TextureLibrary
{
public:

	static void Init();

	static void Destroy();

	static RefCounted<Texture2D> CreateTexture2D(const Size& size, TextureFormat format, bool useMipMap = false);

    static RefCounted<RenderTexture> CreateRenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount = 1, bool useMipMap = false);

	static void ClearCache();

private:

	static inline TArray<RefCounted<Texture2D>> mTexture2DCache;
    static inline TArray<RefCounted<RenderTexture>> mRenderTextureCache;

};

} // namespace Gleam
