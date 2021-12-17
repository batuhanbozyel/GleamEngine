#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#define VOLK_IMPLEMENTATION
#include <SDL_vulkan.h>

#include "VulkanUtils.h"
#include "Core/ApplicationConfig.h"
#include "Renderer/GraphicsContext.h"

using namespace Gleam;

struct VulkanContext
{
	VkInstance Instance;
#ifdef GDEBUG
	VkDebugUtilsMessengerEXT DebugMessenger;
#endif
	VkSurfaceKHR Surface;
	TArray<VkExtensionProperties> Extensions;
	VkDevice Device;
	VkPhysicalDevice PhysicalDevice;
	VkQueue Queue;
	TArray<VkExtensionProperties> DeviceExtensions;
} static sContext;


VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
	VkDebugUtilsMessageTypeFlagsEXT type,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		GLEAM_CORE_ERROR("Vulkan debug error: {0}", pCallbackData->pMessage);
		GLEAM_ASSERT(false);
	}
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		GLEAM_CORE_WARN("Vulkan debug warning: {0}", pCallbackData->pMessage);
	}
	else if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		GLEAM_CORE_INFO("Vulkan debug info: {0}", pCallbackData->pMessage);
	}
	else
	{
		GLEAM_CORE_TRACE("Vulkan debug trace: {0}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

static void CreateInstance(const TString& appName, const Version& appVersion)
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
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &sContext.Instance));
	volkLoadInstance(sContext.Instance);
}

static void CreateDebugMessenger()
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

	VK_CHECK(vkCreateDebugUtilsMessengerEXT(sContext.Instance, &debugInfo, nullptr, &sContext.DebugMessenger));
}

static void CreateDevice()
{
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(sContext.Instance, &physicalDeviceCount, nullptr));

	TArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(sContext.Instance, &physicalDeviceCount, physicalDevices.data()));

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
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, sContext.Surface, &supportsPresent);

			if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supportsPresent)
			{
				sContext.PhysicalDevice = physicalDevice;
				queueFamilyIndex = i;
				goto PHYSICAL_DEVICE_FOUND;
			}
		}
	}

PHYSICAL_DEVICE_FOUND:

	GLEAM_ASSERT(queueFamilyIndex >= 0, "Vulkan physical device could not found!");

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(sContext.PhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	sContext.DeviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(sContext.PhysicalDevice, nullptr, &deviceExtensionCount, sContext.DeviceExtensions.data()));

	float queuePriority = 1.0f;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

	TArray<const char*> requiredDeviceExtension{"VK_KHR_swapchain"};
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(sContext.PhysicalDevice, &deviceCreateInfo, nullptr, &sContext.Device));
	volkLoadDevice(sContext.Device);

	vkGetDeviceQueue(sContext.Device, queueFamilyIndex, 0, &sContext.Queue);
}

void GraphicsContext::Create(SDL_Window* window, const TString& appName, const Version& appVersion)
{
	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan meta-loader failed to load entry points!");

	CreateInstance(appName, appVersion);

	// Get available extensions
	uint32_t extensionCount;
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	sContext.Extensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, sContext.Extensions.data()));
	
#ifdef GDEBUG
	CreateDebugMessenger();
#endif

	// Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(window, sContext.Instance, &sContext.Surface);
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan surface create failed!");

	CreateDevice();

	GLEAM_CORE_INFO("Vulkan graphics context created.");
}

void GraphicsContext::Destroy()
{
	vkDestroySurfaceKHR(sContext.Instance, sContext.Surface, nullptr);
	vkDestroyDevice(sContext.Device, nullptr);
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(sContext.Instance, sContext.DebugMessenger, nullptr);
#endif
	vkDestroyInstance(sContext.Instance, nullptr);

	GLEAM_CORE_INFO("Vulkan graphics context destroyed.");
}

GraphicsDevice GraphicsContext::GetDevice()
{
	return sContext.Device;
}

#endif