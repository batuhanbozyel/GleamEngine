#pragma once
#include "RendererConfig.h"
#include "TextureFormat.h"

namespace Gleam {

#ifdef USE_METAL_RENDERER
struct MetalFrameObject;
using FrameObject = MetalFrameObject;
#else
struct VulkanFrameObject;
using FrameObject = VulkanFrameObject;
#endif

class Swapchain
{
public:

	Swapchain(const RendererProperties& props);
	~Swapchain();

	void Invalidate();

	const FrameObject& AcquireNextFrame();
	void Present();

	TextureFormat GetFormat() const;

	NativeGraphicsHandle GetGraphicsCommandPool() const;
	NativeGraphicsHandle GetSurface() const;
	const FrameObject& GetCurrentFrame() const;

	bool MultisampleEnabled() const
	{
		return mProperties.msaa > 1;
	}

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

private:

	uint32_t mCurrentFrameIndex = 0;

	RendererProperties mProperties;

};


} // namespace Gleam