#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#define VOLK_IMPLEMENTATION
#include <SDL_vulkan.h>

#include "VulkanUtils.h"
#include "Core/Window.h"
#include "Core/ApplicationConfig.h"
#include "Renderer/RendererContext.h"

using namespace Gleam;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		GLEAM_CORE_ERROR("Vulkan: {0}", pCallbackData->pMessage);
		GLEAM_ASSERT(false);
	}
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		GLEAM_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
	}
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		GLEAM_CORE_INFO("Vulkan: {0}", pCallbackData->pMessage);
	}
	else
	{
		GLEAM_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

/************************************************************************/
/*	CreateInstance                                                      */
/************************************************************************/
void RendererContext::CreateInstance(const TString& appName, const Version& appVersion)
{
	// Get window subsystem extensions
	uint32_t extensionCount;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);
	TArray<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(appVersion.major, appVersion.minor, appVersion.patch);
	appInfo.pEngineName = "Gleam Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(GLEAM_ENGINE_MAJOR_VERSION, GLEAM_ENGINE_MINOR_VERSION, GLEAM_ENGINE_PATCH_VERSION);
	appInfo.apiVersion = VULKAN_API_VERSION;

	VkInstanceCreateInfo createInfo{ VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	createInfo.pApplicationInfo = &appInfo;

	// Get validation layers
#ifdef GDEBUG
	extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	TArray<const char*, 1> validationLayers{ "VK_LAYER_KHRONOS_validation" };
	createInfo.enabledLayerCount = 1;
	createInfo.ppEnabledLayerNames = validationLayers.data();
#else
	createInfo.enabledLayerCount = 0;
	createInfo.ppEnabledLayerNames = nullptr;
#endif
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	// Create instance
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mContext.Instance));
	volkLoadInstance(mContext.Instance);
}

/************************************************************************/
/*	CreateDebugMessenger                                                */
/************************************************************************/
void RendererContext::CreateDebugMessenger()
{
	// Create debug messenger
	VkDebugUtilsMessengerCreateInfoEXT debugInfo{ VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
	debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

	debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
		| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	debugInfo.pfnUserCallback = VulkanDebugCallback;
	debugInfo.pUserData = nullptr;

	VK_CHECK(vkCreateDebugUtilsMessengerEXT(mContext.Instance, &debugInfo, nullptr, &mContext.DebugMessenger));
}

/************************************************************************/
/*	CreateDevice                                                        */
/************************************************************************/
void RendererContext::CreateDevice()
{
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(mContext.Instance, &physicalDeviceCount, nullptr));

	TArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(mContext.Instance, &physicalDeviceCount, physicalDevices.data()));

	uint32_t queueFamilyIndex = -1;
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		uint32_t queueFamilyCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		TArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

		for (uint32_t i = 0; i < queueFamilyCount; i++)
		{
			VkBool32 supportsPresent;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, mContext.Surface, &supportsPresent);

			if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
			{
				mContext.PhysicalDevice = physicalDevice;
				queueFamilyIndex = i;
				goto PHYSICAL_DEVICE_FOUND;
			}
		}
	}

PHYSICAL_DEVICE_FOUND:

	GLEAM_ASSERT(queueFamilyIndex >= 0, "Vulkan: Physical device could not found!");

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.PhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	mContext.DeviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.PhysicalDevice, nullptr, &deviceExtensionCount, mContext.DeviceExtensions.data()));

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

	TArray<const char*> requiredDeviceExtension{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(mContext.PhysicalDevice, &deviceCreateInfo, nullptr, &mContext.Device));
	volkLoadDevice(mContext.Device);

	vkGetDeviceQueue(mContext.Device, queueFamilyIndex, 0, &mContext.Queue);
}

/************************************************************************/
/*	CreateSwapchain                                                     */
/************************************************************************/
void RendererContext::CreateSwapchain(const Window& window, const RendererProperties& props)
{
	vkDeviceWaitIdle(mContext.Device);

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mContext.PhysicalDevice, mContext.Surface, &surfaceCapabilities));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext.PhysicalDevice, mContext.Surface, &surfaceFormatCount, nullptr));
	TArray<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mContext.PhysicalDevice, mContext.Surface, &surfaceFormatCount, surfaceFormats.data()));

	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext.PhysicalDevice, mContext.Surface, &presentModeCount, nullptr));
	TArray<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mContext.PhysicalDevice, mContext.Surface, &presentModeCount, presentModes.data()));

	int width, height;
	SDL_Vulkan_GetDrawableSize(window.GetSDLWindow(), &width, &height);
	VkExtent2D imageExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	imageExtent.width = Math::Clamp(imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	imageExtent.height = Math::Clamp(imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	mProperties.width = imageExtent.width;
	mProperties.height = imageExtent.height;

	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = mContext.Surface;
	if (surfaceCapabilities.minImageCount <= 3 && surfaceCapabilities.maxImageCount >= 3 && props.tripleBufferingEnabled)
	{
		swapchainCreateInfo.minImageCount = 3;
	}
	else if (surfaceCapabilities.minImageCount <= 2 && surfaceCapabilities.maxImageCount >= 2)
	{
		swapchainCreateInfo.minImageCount = 2;
	}
	else
	{
		GLEAM_ASSERT(false, "Vulkan: Neither triple nor double buffering is available!");
	}
	swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent = imageExtent;

	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}
	else
	{
		GLEAM_ASSERT(false, "Vulkan: Swapchain does not support being written to!");
	}

	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = [&]()
	{
		if (!props.vsync)
		{
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		for (const auto& presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentMode;
			}
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}();
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageFormat = [&]()
	{
		for (const auto& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				return VK_FORMAT_B8G8R8A8_UNORM;
			}
			else if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
			{
				return VK_FORMAT_R8G8B8A8_UNORM;
			}
		}
		GLEAM_ASSERT(false, "Vulkan: Swapchain image format could not found!");
		return VK_FORMAT_UNDEFINED;
	}();
	swapchainCreateInfo.oldSwapchain = mContext.Swapchain;
	VkSwapchainKHR newSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(mContext.Device, &swapchainCreateInfo, nullptr, &newSwapchain));

	if (mContext.Swapchain != VK_NULL_HANDLE)
	{
		DestroySwapchain();
	}
	mContext.Swapchain = newSwapchain;

	uint32_t swapchainImageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(mContext.Device, mContext.Swapchain, &swapchainImageCount, nullptr));

	mContext.SwapchainImages.resize(swapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(mContext.Device, mContext.Swapchain, &swapchainImageCount, mContext.SwapchainImages.data()));
}
/************************************************************************/
/*	DestroySwapchain                                                    */
/************************************************************************/
void RendererContext::DestroySwapchain()
{
	for (const auto& swapchainImageView : mContext.SwapchainImageViews)
	{
		vkDestroyImageView(mContext.Device, swapchainImageView, nullptr);
	}
	mContext.SwapchainImageViews.clear();
	vkDestroySwapchainKHR(mContext.Device, mContext.Swapchain, nullptr);
}
/************************************************************************/
/*	RendererContext                                                     */
/************************************************************************/
RendererContext::RendererContext(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props)
{
	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan: Meta-loader failed to load entry points!");

	CreateInstance(appName, appVersion);

	// Get available extensions
	uint32_t extensionCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	mContext.Extensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, mContext.Extensions.data()));
	
#ifdef GDEBUG
	CreateDebugMessenger();
#endif

	// Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(window.GetSDLWindow(), mContext.Instance, &mContext.Surface);
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan: Surface creation failed!");

	CreateDevice();
	CreateSwapchain(window, props);

	EventDispatcher<WindowResizeEvent>::Subscribe([&, this](const WindowResizeEvent& e)
	{
		CreateSwapchain(window, props);
		return false;
	});

	GLEAM_CORE_INFO("Vulkan: Graphics context created.");
}

/************************************************************************/
/*	~RendererContext                                                    */
/************************************************************************/
RendererContext::~RendererContext()
{
	DestroySwapchain();
	vkDestroySurfaceKHR(mContext.Instance, mContext.Surface, nullptr);
	vkDestroyDevice(mContext.Device, nullptr);
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(mContext.Instance, mContext.DebugMessenger, nullptr);
#endif
	vkDestroyInstance(mContext.Instance, nullptr);

	GLEAM_CORE_INFO("Vulkan: Graphics context destroyed.");
}

#endif