#pragma once
#include "RendererConfig.h"

#ifdef USE_VULKAN_RENDERER
#include <volk.h>
#define GraphicsDevice VkDevice
#else
#include <Metal.hpp>
#define GraphicsDevice MTL::Device*
#endif

namespace Gleam {

struct Version;
class Window;

class RendererContext
{
public:

	~RendererContext();

	static void Create(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props)
	{
		Destroy();
		mProperties = props;
		mInstance = new RendererContext(window, appName, appVersion, props);
	}
	static void Destroy()
	{
		if (mInstance)
		{
			delete mInstance;
		}
	}

	static GraphicsDevice GetDevice()
	{
		return mInstance->mContext.Device;
	}

	static const RendererProperties& GetProperties()
	{
		return mProperties;
	}

private:

	RendererContext(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props);

#ifdef USE_VULKAN_RENDERER

	void CreateInstance(const TString& appName, const Version& appVersion);
	void CreateDebugMessenger();
	void CreateDevice();
	void CreateSwapchain(const Window& window, const RendererProperties& props);
	void DestroySwapchain();

	struct
	{
		VkInstance Instance{ VK_NULL_HANDLE };
	#ifdef GDEBUG
		VkDebugUtilsMessengerEXT DebugMessenger{ VK_NULL_HANDLE };
	#endif
		VkSurfaceKHR Surface{ VK_NULL_HANDLE };
		TArray<VkExtensionProperties> Extensions;
		VkDevice Device{ VK_NULL_HANDLE };
		VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
		VkQueue Queue{ VK_NULL_HANDLE };
		TArray<VkExtensionProperties> DeviceExtensions;
		VkSwapchainKHR Swapchain{ VK_NULL_HANDLE };
		VkFormat SwapchainImageFormat;
		TArray<VkImage> SwapchainImages;
		TArray<VkImageView> SwapchainImageViews;
	} mContext;
#else
	struct
	{
		MTL::Device* Device;
	} mContext;
#endif

	static inline RendererProperties mProperties;
	static inline RendererContext* mInstance{ nullptr };
};

} // namespace Gleam
