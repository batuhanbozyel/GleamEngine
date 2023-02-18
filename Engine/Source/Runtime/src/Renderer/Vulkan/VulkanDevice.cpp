#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanDevice.h"
#include "VulkanPipelineStateManager.h"

#include "Renderer/RendererContext.h"
#include "Core/ApplicationConfig.h"

#include <SDL_vulkan.h>

using namespace Gleam;

void RendererContext::ConfigureBackend(const TString& appName, const Version& appVersion, const RendererConfig& config)
{
    VulkanDevice::Init(appName, appVersion, config);
	mConfiguration = config;
	mConfiguration.format = VulkanDevice::GetSwapchain().GetFormat();
}

void RendererContext::DestroyBackend()
{
    VulkanDevice::Destroy();
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
												   VkDebugUtilsMessageTypeFlagsEXT type,
												   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
												   void* pUserData)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
	{
		GLEAM_CORE_ERROR("Vulkan: {0}", pCallbackData->pMessage);
		GLEAM_ASSERT(false);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		GLEAM_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
	}
	else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
	{
		GLEAM_CORE_INFO("Vulkan: {0}", pCallbackData->pMessage);
	}
	else
	{
		GLEAM_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
	}
	return VK_FALSE;
}

void VulkanDevice::Init(const TString& appName, const Version& appVersion, const RendererConfig& config)
{
	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan: Meta-loader failed to load entry points!");

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
	VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mInstance));
	volkLoadInstanceOnly(mInstance);

#ifdef GDEBUG
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

	VK_CHECK(vkCreateDebugUtilsMessengerEXT(mInstance, &debugInfo, nullptr, &mDebugMessenger));
#endif

	// Get available extensions
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	mExtensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, mExtensions.data()));

	// Create device
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, nullptr));

	TArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(mInstance, &physicalDeviceCount, physicalDevices.data()));

	mPhysicalDevice = physicalDevices[0];
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			mPhysicalDevice = physicalDevice;
			break;
		}
	}

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, nullptr);

	TArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(mPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	mDeviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, mDeviceExtensions.data()));

	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &mMemoryProperties);

	// Create swapchain
	mSwapchain.Initialize(config);

	// Get physical device queues
	bool mainQueueFound = false;
	bool computeQueueFound = false;
	bool transferQueueFound = false;

	float queuePriority = 1.0f;
	TArray<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
	VkDeviceQueueCreateInfo deviceQueueCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
	deviceQueueCreateInfo.queueCount = 1;
	deviceQueueCreateInfo.pQueuePriorities = &queuePriority;
	for (uint32_t i = 0; i < queueFamilyProperties.size(); i++)
	{
		deviceQueueCreateInfo.queueFamilyIndex = i;
		bool queueFamilySupportsGraphics = queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool queueFamiySupportsCompute = queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT;
		bool queueFamilySupportsTransfer = queueFamilyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT;

		if (queueFamilySupportsGraphics &&
			queueFamiySupportsCompute &&
			queueFamilySupportsTransfer &&
			!mainQueueFound)
		{
			mainQueueFound = true;
			mGraphicsQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

			VkBool32 mainQueueSupportsPresent = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(mPhysicalDevice, i, mSwapchain.GetSurface(), &mainQueueSupportsPresent);
			GLEAM_ASSERT(mainQueueSupportsPresent, "Vulkan: Main queue does not support presentation!");
		}
		else if (queueFamiySupportsCompute &&
			queueFamilySupportsTransfer &&
			!queueFamilySupportsGraphics &&
			!computeQueueFound)
		{
			computeQueueFound = true;
			mComputeQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
		else if (queueFamilySupportsTransfer &&
			!queueFamiySupportsCompute &&
			!queueFamilySupportsGraphics &&
			!transferQueueFound)
		{
			transferQueueFound = true;
			mTransferQueue.index = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
	}

	TArray<const char*> requiredDeviceExtension
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME
	};
    
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(deviceQueueCreateInfos.size());;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mHandle));
	volkLoadDevice(mHandle);

	GLEAM_ASSERT(mainQueueFound, "Vulkan: Main queue could not found!");
	vkGetDeviceQueue(mHandle, mGraphicsQueue.index, 0, &mGraphicsQueue.handle);
	if (computeQueueFound)
	{
		vkGetDeviceQueue(mHandle, mComputeQueue.index, 0, &mComputeQueue.handle);
	}
	else
	{
		mComputeQueue = mGraphicsQueue;
		GLEAM_CORE_WARN("Vulkan: Async compute queue family could not found, mapping to main queue.");
	}
	if (transferQueueFound)
	{
		vkGetDeviceQueue(mHandle, mTransferQueue.index, 0, &mTransferQueue.handle);
	}
	else
	{
		mTransferQueue = mGraphicsQueue;
		GLEAM_CORE_WARN("Vulkan: Transfer queue family could not found, mapping to main queue.");
	}

	// Create swapchain
	mSwapchain.InvalidateAndCreate();

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK(vkCreatePipelineCache(mHandle, &pipelineCacheCreateInfo, nullptr, &mPipelineCache));

	// Create command pools
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	mGraphicsCommandPools.resize(mSwapchain.GetMaxFramesInFlight());
	mTransferCommandPools.resize(mSwapchain.GetMaxFramesInFlight());
	for (uint32_t i = 0; i < mSwapchain.GetMaxFramesInFlight(); i++)
	{
		commandPoolCreateInfo.queueFamilyIndex = mGraphicsQueue.index;
		VK_CHECK(vkCreateCommandPool(mHandle, &commandPoolCreateInfo, nullptr, &mGraphicsCommandPools[i]));

		commandPoolCreateInfo.queueFamilyIndex = mTransferQueue.index;
		VK_CHECK(vkCreateCommandPool(mHandle, &commandPoolCreateInfo, nullptr, &mTransferCommandPools[i]));
	}

	GLEAM_CORE_INFO("Vulkan: Graphics device created.");
}

void VulkanDevice::Destroy()
{
	VK_CHECK(vkDeviceWaitIdle(mHandle));

	VulkanPipelineStateManager::Clear();

	// Destroy pipeline cache
	vkDestroyPipelineCache(mHandle, mPipelineCache, nullptr);

	// Destroy command pools
	for (uint32_t i = 0; i < mSwapchain.GetMaxFramesInFlight(); i++)
	{
		vkDestroyCommandPool(mHandle, mGraphicsCommandPools[i], nullptr);
		vkDestroyCommandPool(mHandle, mTransferCommandPools[i], nullptr);
	}
	mGraphicsCommandPools.clear();
	mTransferCommandPools.clear();

	// Destroy swapchain
	VkSurfaceKHR surface = mSwapchain.GetSurface();
	mSwapchain.Destroy();

	// Destroy surface
	vkDestroySurfaceKHR(mInstance, surface, nullptr);

	// Destroy device
	vkDestroyDevice(mHandle, nullptr);

	// Destroy instance
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(mInstance, mDebugMessenger, nullptr);
#endif
	vkDestroyInstance(mInstance, nullptr);

	GLEAM_CORE_INFO("Vulkan: Graphics device destroyed.");
}

const VulkanQueue& VulkanDevice::GetGraphicsQueue()
{
	return mGraphicsQueue;
}

const VulkanQueue& VulkanDevice::GetComputeQueue()
{
	return mComputeQueue;
}

const VulkanQueue& VulkanDevice::GetTransferQueue()
{
	return mTransferQueue;
}

VulkanSwapchain& VulkanDevice::GetSwapchain()
{
	return mSwapchain;
}

VkDevice VulkanDevice::GetHandle()
{
	return mHandle;
}

VkInstance VulkanDevice::GetInstance()
{
	return mInstance;
}

VkPhysicalDevice VulkanDevice::GetPhysicalDevice()
{
	return mPhysicalDevice;
}

VkCommandPool VulkanDevice::GetGraphicsCommandPool(uint32_t index)
{
	return mGraphicsCommandPools[index];
}

VkCommandPool VulkanDevice::GetTransferCommandPool(uint32_t index)
{
	return mTransferCommandPools[index];
}

uint32_t VulkanDevice::GetMemoryTypeForProperties(uint32_t memoryTypeBits, uint32_t properties)
{
	for (uint32_t i = 0; i < mMemoryProperties.memoryTypeCount; i++)
	{
		if ((memoryTypeBits & BIT(i)) && (mMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	GLEAM_ASSERT(false, "Vulkan: Vertex Buffer suitable memory type could not found!");
	return 0u;
}
#endif
