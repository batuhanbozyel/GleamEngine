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
/*	RendererContext                                                     */
/************************************************************************/
RendererContext::RendererContext(const Window& window, const TString& appName, const Version& appVersion, const RendererProperties& props)
{
	mProperties = props;

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
	CreateSwapchain(window.GetSDLWindow());
	CreateSyncObjects();
	CreateFrameObjects();

	GLEAM_CORE_INFO("Vulkan: Graphics context created.");
}
/************************************************************************/
/*	~RendererContext                                                    */
/************************************************************************/
RendererContext::~RendererContext()
{
	vkDeviceWaitIdle(mContext.Device);
	for (uint32_t i = 0; i < mContext.FrameBufferingCount; i++)
	{
		vkDestroyFence(mContext.Device, mContext.FrameImageAcquireFences[i], nullptr);
		vkDestroyCommandPool(mContext.Device, mContext.FrameCommandPools[i], nullptr);
	}
	mContext.FrameImageAcquireFences.clear();
	mContext.FrameCommandPools.clear();
	DestroySwapchain();
	vkDestroySurfaceKHR(mContext.Instance, mContext.Surface, nullptr);
	vkDestroySemaphore(mContext.Device, mContext.ImageAcquireSemaphore, nullptr);
	vkDestroySemaphore(mContext.Device, mContext.ImageReleaseSemaphore, nullptr);
	vkDestroyDevice(mContext.Device, nullptr);
#ifdef GDEBUG
	vkDestroyDebugUtilsMessengerEXT(mContext.Instance, mContext.DebugMessenger, nullptr);
#endif
	vkDestroyInstance(mContext.Instance, nullptr);

	GLEAM_CORE_INFO("Vulkan: Graphics context destroyed.");
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

	mContext.PhysicalDevice = physicalDevices[0];
	for (VkPhysicalDevice physicalDevice : physicalDevices)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			mContext.PhysicalDevice = physicalDevice;
			break;
		}
	}

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(mContext.PhysicalDevice, &queueFamilyCount, nullptr);

	TArray<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(mContext.PhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t deviceExtensionCount;
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.PhysicalDevice, nullptr, &deviceExtensionCount, nullptr));
	mContext.DeviceExtensions.resize(deviceExtensionCount);
	VK_CHECK(vkEnumerateDeviceExtensionProperties(mContext.PhysicalDevice, nullptr, &deviceExtensionCount, mContext.DeviceExtensions.data()));

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
			mContext.GraphicsQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);

			VkBool32 mainQueueSupportsPresent = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(mContext.PhysicalDevice, i, mContext.Surface, &mainQueueSupportsPresent);
			GLEAM_ASSERT(mainQueueSupportsPresent, "Vulkan: Main queue does not support presentation!");
		}
		else if (queueFamiySupportsCompute &&
				 queueFamilySupportsTransfer &&
				 !queueFamilySupportsGraphics &&
				 !computeQueueFound)
		{
			computeQueueFound = true;
			mContext.ComputeQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
		else if (queueFamilySupportsTransfer &&
				 !queueFamiySupportsCompute &&
				 !queueFamilySupportsGraphics &&
				 !transferQueueFound)
		{
			transferQueueFound = true;
			mContext.TransferQueueFamilyIndex = i;
			deviceQueueCreateInfos.push_back(deviceQueueCreateInfo);
		}
	}

	TArray<const char*> requiredDeviceExtension{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = queueFamilyCount;
	deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfos.data();
	deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtension.size());
	deviceCreateInfo.ppEnabledExtensionNames = requiredDeviceExtension.data();

	VK_CHECK(vkCreateDevice(mContext.PhysicalDevice, &deviceCreateInfo, nullptr, &mContext.Device));
	volkLoadDevice(mContext.Device);

	GLEAM_ASSERT(mainQueueFound, "Vulkan: Main queue could not found!");
	vkGetDeviceQueue(mContext.Device, mContext.GraphicsQueueFamilyIndex, 0, &mContext.GraphicsQueue);
	if (computeQueueFound)
	{
		vkGetDeviceQueue(mContext.Device, mContext.ComputeQueueFamilyIndex, 0, &mContext.ComputeQueue);
	}
	else
	{
		mContext.ComputeQueue = mContext.GraphicsQueue;
		mContext.ComputeQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
		GLEAM_CORE_WARN("Vulkan: Async compute queue family could not found, mapping to main queue.");
	}
	if (transferQueueFound)
	{
		vkGetDeviceQueue(mContext.Device, mContext.TransferQueueFamilyIndex, 0, &mContext.TransferQueue);
	}
	else
	{
		mContext.TransferQueue = mContext.GraphicsQueue;
		mContext.TransferQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
		GLEAM_CORE_WARN("Vulkan: Transfer queue family could not found, mapping to main queue.");
	}
}
/************************************************************************/
/*	CreateSwapchain                                                     */
/************************************************************************/
void RendererContext::CreateSwapchain(SDL_Window* window)
{
	vkDeviceWaitIdle(mContext.Device);

	// Get surface information
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
	SDL_Vulkan_GetDrawableSize(window, &width, &height);
	VkExtent2D imageExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	imageExtent.width = Math::Clamp(imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	imageExtent.height = Math::Clamp(imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	mProperties.width = imageExtent.width;
	mProperties.height = imageExtent.height;

	VkFormat imageFormat = [&]()
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

	// Create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = mContext.Surface;
	if (surfaceCapabilities.minImageCount <= 3 && surfaceCapabilities.maxImageCount >= 3 && mProperties.tripleBufferingEnabled)
	{
		mContext.FrameBufferingCount = 3;
		swapchainCreateInfo.minImageCount = 3;
		GLEAM_CORE_TRACE("Vulkan: Triple buffering enabled.");
	}
	else if (surfaceCapabilities.minImageCount <= 2 && surfaceCapabilities.maxImageCount >= 2)
	{
		mContext.FrameBufferingCount = 2;
		swapchainCreateInfo.minImageCount = 2;
		if (mProperties.tripleBufferingEnabled)
		{
			mProperties.tripleBufferingEnabled = false;
		}
		GLEAM_CORE_TRACE("Vulkan: Double buffering enabled.");
	}
	else
	{
		GLEAM_ASSERT(false, "Vulkan: Neither triple nor double buffering is available!");
	}
	swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapchainCreateInfo.imageExtent = imageExtent;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT)
	{
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	}
	if (surfaceCapabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
	{
		swapchainCreateInfo.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	}

	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchainCreateInfo.presentMode = [&]()
	{
	#ifndef PLATFORM_ANDROID // use FIFO for mobile to reduce power consumption
		if (!mProperties.vsync)
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
	#endif
		return VK_PRESENT_MODE_FIFO_KHR;
	}();
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageFormat = imageFormat;
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

	// Create image views
	mContext.SwapchainImageViews.resize(swapchainImageCount);
	VkImageViewUsageCreateInfo imageViewUsageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
	imageViewUsageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.pNext = &imageViewUsageCreateInfo;
	imageViewCreateInfo.format = imageFormat;
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
	};

	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		imageViewCreateInfo.image = mContext.SwapchainImages[i];
		VK_CHECK(vkCreateImageView(mContext.Device, &imageViewCreateInfo, nullptr, &mContext.SwapchainImageViews[i]));
	}

	// Create render pass
	VkAttachmentDescription attachmentDesc{};
	attachmentDesc.format = imageFormat;
	attachmentDesc.samples = mProperties.multisampleEnabled ? static_cast<VkSampleCountFlagBits>(BIT(mProperties.msaa - 1)) : VK_SAMPLE_COUNT_1_BIT;
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference attachmentRef{};
	attachmentRef.attachment = 0;
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &attachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;

	VK_CHECK(vkCreateRenderPass(mContext.Device, &renderPassCreateInfo, nullptr, &mContext.SwapchainRenderPass));

	// Create framebuffer
	mContext.SwapchainFramebuffers.resize(swapchainImageCount);

	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.width = mProperties.width;
	framebufferCreateInfo.height = mProperties.height;
	framebufferCreateInfo.renderPass = mContext.SwapchainRenderPass;
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.layers = 1;
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		framebufferCreateInfo.pAttachments = &mContext.SwapchainImageViews[i];
		VK_CHECK(vkCreateFramebuffer(mContext.Device, &framebufferCreateInfo, nullptr, &mContext.SwapchainFramebuffers[i]));
	}
}
/************************************************************************/
/*	CreateSyncObjects                                                    */
/************************************************************************/
void RendererContext::CreateSyncObjects()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VK_CHECK(vkCreateSemaphore(mContext.Device, &semaphoreCreateInfo, nullptr, &mContext.ImageAcquireSemaphore));
	VK_CHECK(vkCreateSemaphore(mContext.Device, &semaphoreCreateInfo, nullptr, &mContext.ImageReleaseSemaphore));
}
/************************************************************************/
/*	CreateFrameObjects                                                    */
/************************************************************************/
void RendererContext::CreateFrameObjects()
{
	mContext.FrameImageAcquireFences.resize(mContext.FrameBufferingCount);
	mContext.FrameCommandBuffers.resize(mContext.FrameBufferingCount);
	mContext.FrameCommandPools.resize(mContext.FrameBufferingCount);
	for (uint32_t i = 0; i < mContext.FrameBufferingCount; i++)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolCreateInfo.queueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VK_CHECK(vkCreateCommandPool(mContext.Device, &commandPoolCreateInfo, nullptr, &mContext.FrameCommandPools[i]));

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferAllocateInfo.commandBufferCount = 1;
		commandBufferAllocateInfo.commandPool = mContext.FrameCommandPools[i];
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		VK_CHECK(vkAllocateCommandBuffers(mContext.Device, &commandBufferAllocateInfo, &mContext.FrameCommandBuffers[i]));

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK(vkCreateFence(mContext.Device, &fenceCreateInfo, nullptr, &mContext.FrameImageAcquireFences[i]));
	}
}
/************************************************************************/
/*	DestroySwapchain                                                    */
/************************************************************************/
void RendererContext::DestroySwapchain()
{
	for (uint32_t i = 0; i < mContext.SwapchainFramebuffers.size(); i++)
	{
		vkDestroyFramebuffer(mContext.Device, mContext.SwapchainFramebuffers[i], nullptr);
		vkDestroyImageView(mContext.Device, mContext.SwapchainImageViews[i], nullptr);
	}
	mContext.SwapchainFramebuffers.clear();
	mContext.SwapchainImageViews.clear();
	vkDestroyRenderPass(mContext.Device, mContext.SwapchainRenderPass, nullptr);
	vkDestroySwapchainKHR(mContext.Device, mContext.Swapchain, nullptr);
}
/************************************************************************/
/*	SwapBuffersImpl                                                     */
/************************************************************************/
void RendererContext::SwapBuffersImpl(SDL_Window* window)
{
	VK_CHECK(vkWaitForFences(mContext.Device, 1, &mContext.FrameImageAcquireFences[mContext.CurrentFrameIndex], VK_TRUE, UINT64_MAX));

	VkResult result = vkAcquireNextImageKHR(mContext.Device, mContext.Swapchain, UINT64_MAX, mContext.ImageAcquireSemaphore, VK_NULL_HANDLE, &mContext.CurrentImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		CreateSwapchain(window);
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Swapbuffers failed to acquire next image!");
	}

	VkCommandBufferBeginInfo commandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkClearColorValue clearColor{ 1.0f, 0.8f, 0.4f, 1.0f };

	VkImageSubresourceRange imageSubresourceRange;
	imageSubresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageSubresourceRange.baseArrayLayer = 0;
	imageSubresourceRange.baseMipLevel = 0;
	imageSubresourceRange.layerCount = 1;
	imageSubresourceRange.levelCount = 1;

	VkImageMemoryBarrier imageMemoryBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	imageMemoryBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.srcQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
	imageMemoryBarrier.dstQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
	imageMemoryBarrier.image = mContext.SwapchainImages[mContext.CurrentImageIndex];
	imageMemoryBarrier.subresourceRange = imageSubresourceRange;

	VK_CHECK(vkBeginCommandBuffer(mContext.FrameCommandBuffers[mContext.CurrentFrameIndex], &commandBufferBeginInfo));
	vkCmdPipelineBarrier(mContext.FrameCommandBuffers[mContext.CurrentFrameIndex], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

	vkCmdClearColorImage(mContext.FrameCommandBuffers[mContext.CurrentFrameIndex], mContext.SwapchainImages[mContext.CurrentImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clearColor, 1, &imageSubresourceRange);

	imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	imageMemoryBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	imageMemoryBarrier.srcQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
	imageMemoryBarrier.dstQueueFamilyIndex = mContext.GraphicsQueueFamilyIndex;
	imageMemoryBarrier.image = mContext.SwapchainImages[mContext.CurrentImageIndex];
	imageMemoryBarrier.subresourceRange = imageSubresourceRange;

	vkCmdPipelineBarrier(mContext.FrameCommandBuffers[mContext.CurrentFrameIndex], VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
	VK_CHECK(vkEndCommandBuffer(mContext.FrameCommandBuffers[mContext.CurrentFrameIndex]));

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &mContext.ImageAcquireSemaphore;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mContext.FrameCommandBuffers[mContext.CurrentFrameIndex];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mContext.ImageReleaseSemaphore;

	VK_CHECK(vkResetFences(mContext.Device, 1, &mContext.FrameImageAcquireFences[mContext.CurrentFrameIndex]));
	VK_CHECK(vkQueueSubmit(mContext.GraphicsQueue, 1, &submitInfo, mContext.FrameImageAcquireFences[mContext.CurrentFrameIndex]));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mContext.ImageReleaseSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mContext.Swapchain;
	presentInfo.pImageIndices = &mContext.CurrentImageIndex;
	result = vkQueuePresentKHR(mContext.GraphicsQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		CreateSwapchain(window);
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Image presentation failed!");
	}
	
	mContext.CurrentFrameIndex = (mContext.CurrentFrameIndex + 1) % mContext.FrameBufferingCount;
}
/************************************************************************/
/*	ClearColorImpl                                                      */
/************************************************************************/
void RendererContext::ClearColorImpl(const Color& color)
{
	
}
#endif