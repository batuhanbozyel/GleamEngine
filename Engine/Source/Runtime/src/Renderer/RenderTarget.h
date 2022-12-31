#pragma once
#include "Texture.h"

namespace Gleam {


class RenderTarget final
{
public:
    
	RenderTarget(TextureFormat format, const Size& size, uint32_t samples = 1, bool useMipMap = false)
    {
        auto renderBuffer = TextureLibrary::CreateRenderTexture(size, format, samples, useMipMap);
		renderBuffer->Lock();

		if (IsDepthStencilFormat(format))
			mDepthBuffer = renderBuffer;
		else
			mColorBuffers.push_back(renderBuffer);
    }

	RenderTarget(TextureFormat colorFormat, TextureFormat depthFormat, const Size& size, uint32_t samples = 1, bool useMipMap = false)
    {
        auto renderBuffer = TextureLibrary::CreateRenderTexture(size, colorFormat, samples, useMipMap);
		renderBuffer->Lock();
		mColorBuffers.push_back(renderBuffer);

		mDepthBuffer = TextureLibrary::CreateRenderTexture(size, depthFormat, samples, useMipMap);
		mDepthBuffer->Lock();
    }

    RenderTarget(const TArray<TextureFormat>& formats, const Size& size, uint32_t samples = 1, bool useMipMap = false)
    {
        for (auto format : formats)
        {
            auto renderBuffer = TextureLibrary::CreateRenderTexture(size, format, samples, useMipMap);
            renderBuffer->Lock();
            
            if (IsDepthStencilFormat(format))
                mDepthBuffer = renderBuffer;
            else
                mColorBuffers.push_back(renderBuffer);
        }
    }

    ~RenderTarget()
    {
        if (HasDepthBuffer())
            mDepthBuffer->Release();
        
        for (auto& colorAttachment : mColorBuffers)
            colorAttachment->Release();
    }
    
    const RefCounted<RenderTexture>& GetDepthBuffer() const
    {
        return mDepthBuffer;
    }
    
    const TArray<RefCounted<RenderTexture>>& GetColorBuffers() const
    {
        return mColorBuffers;
    }
    
    bool HasDepthBuffer() const
    {
        return mDepthBuffer != nullptr;
    }
    
private:
    
    RefCounted<RenderTexture> mDepthBuffer;
    TArray<RefCounted<RenderTexture>> mColorBuffers;
    
};

} // namespace Gleam

