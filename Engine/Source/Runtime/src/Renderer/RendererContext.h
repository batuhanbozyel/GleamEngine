#pragma once
#include "RendererConfig.h"

namespace Gleam {

#ifdef USE_METAL_RENDERER
struct MetalFrameObject;
using FrameObject = MetalFrameObject;
#else
struct VulkanFrameObject;
using FrameObject = VulkanFrameObject;
#endif
struct Version;

class RendererContext
{
public:

	RendererContext(const TString& appName, const Version& appVersion, const RendererProperties& props);
	~RendererContext();
    
	const FrameObject& AcquireNextFrame();
	const FrameObject& GetCurrentFrame() const;
	void Present();

	handle_t GetDevice() const;

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

private:
    
	void InvalidateSwapchain();
    
	RendererProperties mProperties;
	uint32_t mCurrentFrameIndex = 0;

};

} // namespace Gleam
