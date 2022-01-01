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

	static void ClearColor(const Color& color)
	{
		mInstance->ClearColorImpl(color);
	}

	static void SwapBuffers(SDL_Window* window)
	{
		mInstance->SwapBuffersImpl(window);
	}

	static const RendererProperties& GetProperties()
	{
		return mProperties;
	}

private:

	RendererContext(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props);

	void SwapBuffersImpl(SDL_Window* window);
	void ClearColorImpl(const Color& color);

#ifdef USE_VULKAN_RENDERER

	void CreateInstance(const TString& appName, const Version& appVersion);
	void CreateDebugMessenger();
	void CreateDevice();
	void CreateSwapchain(SDL_Window* window);
	void CreateSyncObjects();
	void CreateFrameObjects();
	void DestroySwapchain();

	struct
	{
		VkInstance Instance{ VK_NULL_HANDLE };

		// Debug/Validation layer
	#ifdef GDEBUG
		VkDebugUtilsMessengerEXT DebugMessenger{ VK_NULL_HANDLE };
	#endif

		// Surface
		VkSurfaceKHR Surface{ VK_NULL_HANDLE };
		TArray<VkExtensionProperties> Extensions;

		// Device
		VkDevice Device{ VK_NULL_HANDLE };
		VkPhysicalDevice PhysicalDevice{ VK_NULL_HANDLE };
		VkQueue GraphicsQueue{ VK_NULL_HANDLE };
		uint32_t GraphicsQueueFamilyIndex{ 0 };
		VkQueue ComputeQueue{ VK_NULL_HANDLE };
		uint32_t ComputeQueueFamilyIndex{ 0 };
		VkQueue TransferQueue{ VK_NULL_HANDLE };
		uint32_t TransferQueueFamilyIndex{ 0 };
		TArray<VkExtensionProperties> DeviceExtensions;

		// Swapchain
		VkSwapchainKHR Swapchain{ VK_NULL_HANDLE };
		VkFormat SwapchainImageFormat;
		TArray<VkImage> SwapchainImages;
		TArray<VkImageView> SwapchainImageViews;
		VkRenderPass SwapchainRenderPass;
		TArray<VkFramebuffer> SwapchainFramebuffers;
		uint32_t CurrentImageIndex;

		// Synchronization
		VkSemaphore ImageAcquireSemaphore;
		VkSemaphore ImageReleaseSemaphore;

		// Frame
		TArray<VkCommandPool> FrameCommandPools;
		TArray<VkFence> FrameImageAcquireFences;
		TArray<VkCommandBuffer> FrameCommandBuffers;
		uint32_t FrameBufferingCount{ 0 };
		uint32_t CurrentFrameIndex{ 0 };
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
