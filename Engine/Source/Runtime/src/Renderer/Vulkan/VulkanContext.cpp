#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/RendererContext.h"
#include "VulkanUtils.h"

using namespace Gleam;

struct
{
	// Device
	VkPhysicalDevice physicalDevice{ VK_NULL_HANDLE };
	VkQueue graphicsQueue{ VK_NULL_HANDLE };
	uint32_t graphicsQueueFamilyIndex{ 0 };
	VkQueue computeQueue{ VK_NULL_HANDLE };
	uint32_t computeQueueFamilyIndex{ 0 };
	VkQueue transferQueue{ VK_NULL_HANDLE };
	uint32_t transferQueueFamilyIndex{ 0 };
	TArray<VkExtensionProperties> deviceExtensions;
	VkPhysicalDeviceMemoryProperties memoryProperties;

	// Pipeline cache
	VkPipelineCache pipelineCache;

} mContext;

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT severity,
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
/************************************************************************/
/*	Init                                                                */
/************************************************************************/
void RendererContext::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
	VkResult loadVKResult = volkInitialize();
	GLEAM_ASSERT(loadVKResult == VK_SUCCESS, "Vulkan: Meta-loader failed to load entry points!");

    mSwapchain.reset(new Swapchain(appName, appVersion, props));

	// Create device
	uint32_t physicalDeviceCount;
	VK_CHECK(vkEnumeratePhysicalDevices(As<VkInstance>(mSwapchain->GetLayer()), &physicalDeviceCount, nullptr));

	TArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
	VK_CHECK(vkEnumeratePhysicalDevices(As<VkInstance>(mSwapchain->GetLayer()), &physicalDeviceCount, physicalDevices.data()));

	mContext.physicalDevice = physicalDevices[0];
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			mContext.physicalDevice = physicalDevice;
			break;
		}
	}

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(mContext.physicalDevice, &queueFamilyCount, nullptr);

	TArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(mContext.physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.physicalDevice, nullptr, &deviceExtensionCount, nullptr));
	mContext.deviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.physicalDevice, nullptr, &deviceExtensionCount, mContext.deviceExtensions.data()));

	vkGetPhysicalDeviceMemoryProperties(mContext.physicalDevice, &mContext.memoryProperties);

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
			mContext.graphicsQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

			VkBool32 mainQueueSupportsPresent = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(mContext.physicalDevice, i, As<VkSurfaceKHR>(mSwapchain->GetSurface()), &mainQueueSupportsPresent);
			GLEAM_ASSERT(mainQueueSupportsPresent, "Vulkan: Main queue does not support presentation!");
		}
		else if (queueFamiySupportsCompute &&
			queueFamilySupportsTransfer &&
			!queueFamilySupportsGraphics &&
			!computeQueueFound)
		{
			computeQueueFound = true;
			mContext.computeQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
		else if (queueFamilySupportsTransfer &&
			!queueFamiySupportsCompute &&
			!queueFamilySupportsGraphics &&
			!transferQueueFound)
		{
			transferQueueFound = true;
			mContext.transferQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
	}

	TArray<const char*> requiredDeviceExtension
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
		VK_EXT_SCALAR_BLOCK_LAYOUT_EXTENSION_NAME
	};

	VkPhysicalDeviceScalarBlockLayoutFeatures scalarBlockLayoutExt{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES };
	scalarBlockLayoutExt.scalarBlockLayout = true;

	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.pNext = &scalarBlockLayoutExt;
	deviceCreateInfo.queueCreateInfoCount = queueFamilyCount;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(mContext.physicalDevice, &deviceCreateInfo, nullptr, As<VkDevice*>(&mDevice)));
	volkLoadDevice(As<VkDevice>(mDevice));

	GLEAM_ASSERT(mainQueueFound, "Vulkan: Main queue could not found!");
	vkGetDeviceQueue(As<VkDevice>(mDevice), mContext.graphicsQueueFamilyIndex, 0, &mContext.graphicsQueue);
	if (computeQueueFound)
	{
		vkGetDeviceQueue(As<VkDevice>(mDevice), mContext.computeQueueFamilyIndex, 0, &mContext.computeQueue);
	}
	else
	{
		mContext.computeQueue = mContext.graphicsQueue;
		mContext.computeQueueFamilyIndex = mContext.graphicsQueueFamilyIndex;
		GLEAM_CORE_WARN("Vulkan: Async compute queue family could not found, mapping to main queue.");
	}
	if (transferQueueFound)
	{
		vkGetDeviceQueue(As<VkDevice>(mDevice), mContext.transferQueueFamilyIndex, 0, &mContext.transferQueue);
	}
	else
	{
		mContext.transferQueue = mContext.graphicsQueue;
		mContext.transferQueueFamilyIndex = mContext.graphicsQueueFamilyIndex;
		GLEAM_CORE_WARN("Vulkan: Transfer queue family could not found, mapping to main queue.");
	}

	// Create swapchain
	mSwapchain->InvalidateAndCreate();

	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK(vkCreatePipelineCache(VulkanDevice, &pipelineCacheCreateInfo, nullptr, &mContext.pipelineCache));

	GLEAM_CORE_INFO("Vulkan: Graphics context created.");
}
/************************************************************************/
/*	Destroy                                                             */
/************************************************************************/
void RendererContext::Destroy()
{
	vkDeviceWaitIdle(As<VkDevice>(mDevice));

	// Destroy pipeline cache
	vkDestroyPipelineCache(VulkanDevice, mContext.pipelineCache, nullptr);

	// Destroy swapchain
	mSwapchain.reset();

	// Destroy device
	vkDestroyDevice(As<VkDevice>(mDevice), nullptr);

	GLEAM_CORE_INFO("Vulkan: Graphics context destroyed.");
}
/************************************************************************/
/*	GetPhysicalDevice                                                   */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetPhysicalDevice()
{
	return mContext.physicalDevice;
}
/************************************************************************/
/*	GetGraphicsQueue                                                    */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetGraphicsQueue()
{
	return mContext.graphicsQueue;
}
/************************************************************************/
/*	GetComputeQueue                                                     */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetComputeQueue()
{
	return mContext.computeQueue;
}
/************************************************************************/
/*	GetTransferQueue                                                    */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetTransferQueue()
{
	return mContext.transferQueue;
}
/************************************************************************/
/*	GetGraphicsQueueIndex                                               */
/************************************************************************/
uint32_t RendererContext::GetGraphicsQueueIndex()
{
	return mContext.graphicsQueueFamilyIndex;
}
/************************************************************************/
/*	GetComputeQueueIndex                                                */
/************************************************************************/
uint32_t RendererContext::GetComputeQueueIndex()
{
	return mContext.computeQueueFamilyIndex;
}
/************************************************************************/
/*	GetTransferQueueIndex                                               */
/************************************************************************/
uint32_t RendererContext::GetTransferQueueIndex()
{
	return mContext.computeQueueFamilyIndex;
}
/************************************************************************/
/*	GetMemoryTypeForProperties                                           */
/************************************************************************/
uint32_t RendererContext::GetMemoryTypeForProperties(uint32_t memoryTypeBits, uint32_t properties)
{
	for (uint32_t i = 0; i < mContext.memoryProperties.memoryTypeCount; i++)
	{
		if ((memoryTypeBits & BIT(i)) && (mContext.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	GLEAM_ASSERT(false, "Vulkan: Vertex Buffer suitable memory type could not found!");
	return 0u;
}
#endif
