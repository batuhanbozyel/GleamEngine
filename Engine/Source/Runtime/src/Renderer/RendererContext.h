#pragma once
#include "RendererConfig.h"

#ifdef USE_VULKAN_RENDERER
#include "Vulkan/VulkanUtils.h"
#define GraphicsDevice VkDevice
#define FrameObject VulkanFrameObject
#else
#include "Metal/MetalUtils.h"
#define GraphicsDevice MTL::Device*
#define SwapchainImage MTL::Texture*
#define FrameObject MetalFrameObject
#endif

namespace Gleam {

struct Version;

class RendererContext
{
public:

	RendererContext(const TString& appName, const Version& appVersion, const RendererProperties& props);
	~RendererContext();

	void BeginFrame();
	void ClearScreen(const Color& color) const;
	void EndFrame();

	GraphicsDevice GetDevice() const;
	FrameObject GetCurrentFrameObject() const;

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

private:

#ifdef USE_VULKAN_RENDERER
	void CreateInstance(const TString& appName, const Version& appVersion);
#ifdef GDEBUG
	void CreateDebugMessenger();
#endif
	void CreateDevice();
	void CreateSwapchain();
	void CreateSyncObjects();
	void CreateFrameObjects();
	void DestroySwapchain();
#else
	
#endif

	RendererProperties mProperties;
};

} // namespace Gleam