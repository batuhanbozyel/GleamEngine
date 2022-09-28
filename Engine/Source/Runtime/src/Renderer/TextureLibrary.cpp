#include "gpch.h"
#include "TextureLibrary.h"

using namespace Gleam;

void TextureLibrary::Init()
{

}

void TextureLibrary::Destroy()
{
	ClearCache();
}

void TextureLibrary::ClearCache()
{
    mTexture2DCache.clear();
    mRenderTextureCache.clear();
}

RefCounted<RenderTexture> TextureLibrary::CreateRenderTexture(const Size& size, TextureFormat format, uint32_t sampleCount, bool useMipMap)
{
    for (const auto& renderTexture : mRenderTextureCache)
    {
        if (renderTexture->GetSize() == size &&
            renderTexture->GetFormat() == format &&
            renderTexture->GetSampleCount() == sampleCount &&
            renderTexture->MipMapped() == useMipMap &&
            renderTexture->Available())
        {
            return renderTexture;
        }
    }
    
    auto renderTexture = CreateRef<RenderTexture>(size, format, sampleCount, useMipMap);
    mRenderTextureCache.push_back(renderTexture);
    return renderTexture;
}
