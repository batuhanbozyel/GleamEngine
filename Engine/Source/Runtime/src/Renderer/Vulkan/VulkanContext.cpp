#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include <volk.h>
#include <SDL_vulkan.h>
#include "VulkanUtils.h"

#include "Renderer/GraphicsContext.h"

using namespace Gleam;

struct VulkanContext
{
	VkDevice Device;
	VkInstance Instance;
	VkSurfaceKHR Surface;
	VkPhysicalDevice PhysicalDevice;
	TArray<VkExtensionProperties> Extensions;
#ifdef GDEBUG
	VkDebugUtilsMessengerEXT DebugMessenger;
#endif
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

static void CreateInstance(const TString& appName)
{
	// Get window subsystem extensions
	uint32_t extensionCount;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);
	TArray<const char*> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	VkApplicationInfo appInfo{ VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.pApplicationName = appName.c_str();
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "Gleam Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

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
	/*
		TODO: 
	*/
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 0;
	deviceCreateInfo.pQueueCreateInfos = nullptr;
	deviceCreateInfo.enabledExtensionCount = 0;
	deviceCreateInfo.ppEnabledExtensionNames = nullptr;
	deviceCreateInfo.pEnabledFeatures = nullptr;
	deviceCreateInfo.pNext = nullptr;

	VK_CHECK(vkCreateDevice(sContext.PhysicalDevice, &deviceCreateInfo, nullptr, &sContext.Device));
}

GraphicsContext::GraphicsContext(const TString& appName, const Window& window)
{
	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan meta-loader failed to load entry points!");

	CreateInstance(appName);
	volkLoadInstance(sContext.Instance);

	// Get available extensions
	uint32_t extensionCount;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	sContext.Extensions.resize(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, sContext.Extensions.data());
	
#ifdef GDEBUG
	CreateDebugMessenger();
#endif

	// Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(window.GetSDLWindow(), sContext.Instance, &sContext.Surface);
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan surface create failed!");

	//VK_CHECK(CreateDevice());
	//volkLoadDevice(sContext.Device);

	GLEAM_CORE_INFO("Vulkan graphics context created.");
}

GraphicsContext::~GraphicsContext()
{
	vkDestroySurfaceKHR(sContext.Instance, sContext.Surface, nullptr);
	vkDestroyDevice(sContext.Device, nullptr);
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(sContext.Instance, sContext.DebugMessenger, nullptr);
#endif
	vkDestroyInstance(sContext.Instance, nullptr);

	GLEAM_CORE_INFO("Vulkan graphics context destroyed.");
}

#endif