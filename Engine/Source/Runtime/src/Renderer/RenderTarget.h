#pragma once
#include "Texture.h"

namespace Gleam {

using RenderTargetIdentifier = uint32_t;
static constexpr RenderTargetIdentifier SwapchainTarget = -1;

class RenderTarget final
{
public:
    
	RenderTarget(const TArray<RefCounted<RenderTexture>>& colorBuffers, const RefCounted<RenderTexture>& depthBuffer = nullptr)
		: mColorBuffers(colorBuffers), mDepthBuffer(depthBuffer)
	{
		if (HasDepthBuffer())
			mDepthBuffer->Lock();

		for (auto& colorAttachment : mColorBuffers)
			colorAttachment->Lock();
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

