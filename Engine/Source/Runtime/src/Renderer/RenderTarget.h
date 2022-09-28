#pragma once
#include "TextureLibrary.h"
#include "RenderPassDescriptor.h"

namespace Gleam {

class RenderTarget final
{
public:
    
    RenderTarget(const TArray<TextureFormat>& formats, const Size& size, uint32_t samples = 1, bool useMipMap = false)
    {
        for (uint32_t i = 0; i < formats.size(); i++)
        {
            auto renderBuffer = TextureLibrary::CreateRenderTexture(size, formats[i], samples, useMipMap);
            renderBuffer->Lock();
            
            if (IsDepthStencilFormat(formats[i]))
            {
                mDepthBuffer = renderBuffer;
            }
            else
            {
                mColorBuffers.push_back(renderBuffer);
            }
        }
    }
    
    ~RenderTarget()
    {
        if (HasDepthBuffer())
        {
            mDepthBuffer->Release();
        }
        
        for (auto& colorAttachment : mColorBuffers)
        {
            colorAttachment->Release();
        }
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

