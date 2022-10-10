#pragma once
#include "RendererConfig.h"
#include "TextureFormat.h"

namespace Gleam {

struct Version;

class Swapchain final : public GraphicsObject
{
public:

	Swapchain(const RendererProperties& props, NativeGraphicsHandle instance);
	~Swapchain();

	void InvalidateAndCreate();

	NativeGraphicsHandle AcquireNextDrawable();
	void Present(NativeGraphicsHandle commandBuffer);

    TextureFormat GetFormat() const;

	NativeGraphicsHandle GetDrawable() const;

	DispatchSemaphore GetImageAcquireSemaphore() const;

	DispatchSemaphore GetImageReleaseSemaphore() const;

	uint32_t GetFrameIndex() const
	{
		return mCurrentFrameIndex;
	}

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}
    
    const Size& GetSize() const
    {
        return mSize;
    }

	void* GetSurface() const
	{
		return mSurface;
	}

private:

	uint32_t mCurrentFrameIndex = 0;

    Size mSize = Size::zero;
    
	RendererProperties mProperties;
    
    void* mSurface = nullptr;

};

} // namespace Gleam
