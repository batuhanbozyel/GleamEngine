#pragma once
#include "RendererConfig.h"
#include "TextureFormat.h"
#include "CommandBuffer.h"

namespace Gleam {

struct Version;

class Swapchain : public GraphicsObject
{
public:

	Swapchain(const RendererProperties& props, NativeGraphicsHandle instance);
	~Swapchain();

	void InvalidateAndCreate();

	NativeGraphicsHandle AcquireNextDrawable();
	void Present(NativeGraphicsHandle commandBuffer);

    TextureFormat GetFormat() const;

	NativeGraphicsHandle GetCurrentDrawable() const;

	uint32_t GetCurrentFrameIndex() const
	{
		return mCurrentFrameIndex;
	}

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

	void* GetSurface() const
	{
		return mSurface;
	}

private:

	uint32_t mCurrentFrameIndex = 0;

	RendererProperties mProperties;
    
    void* mSurface = nullptr;

};

} // namespace Gleam
