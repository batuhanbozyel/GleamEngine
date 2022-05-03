#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Swapchain.h"
#include "VulkanUtils.h"

#include "Core/Window.h"
#include "Core/Application.h"

#include <SDL_vulkan.h>

using namespace Gleam;

struct
{
    // Instance
    VkInstance instance{ VK_NULL_HANDLE };

    // Debug/Validation layer
#ifdef GDEBUG
    VkDebugUtilsMessengerEXT debugMessenger{ VK_NULL_HANDLE };
#endif
    
	// Surface
	TArray<VkExtensionProperties> extensions;
	TArray<VkSurfaceFormatKHR> surfaceFormats;
	TArray<VkPresentModeKHR> presentModes;

	// Swapchain
	TArray<VkImage> images;
	TArray<VkImageView> imageViews;
	TArray<VkFramebuffer> framebuffers;
	VkFormat imageFormat;

	// Synchronization
	TArray<VkSemaphore> imageAcquireSemaphores;
	TArray<VkSemaphore> imageReleaseSemaphores;
	TArray<VkFence> imageAcquireFences;

	// Frame
	TArray<VkCommandPool> commandPools;
	TArray<VkCommandBuffer> commandBuffers;
	VulkanFrameObject currentFrame;
} mContext;

Swapchain::Swapchain(const TString& appName, const Version& appVersion, const RendererProperties& props)
	: mProperties(props)
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
    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &mContext.instance));
    volkLoadInstanceOnly(mContext.instance);

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

    VK_CHECK(vkCreateDebugUtilsMessengerEXT(mContext.instance, &debugInfo, nullptr, &mContext.debugMessenger));
#endif
    
	// Get available extensions
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
	mContext.extensions.resize(extensionCount);
	VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, mContext.extensions.data()));

	// Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(ApplicationInstance.GetActiveWindow().GetSDLWindow(), As<VkInstance>(RendererContext::GetEntryPoint()), As<VkSurface*>(&mSurface));
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan: Surface creation failed!");
}

Swapchain::~Swapchain()
{
	// Destroy swapchain
	for (uint32_t i = 0; i < mContext.framebuffers.size(); i++)
	{
		vkDestroyFramebuffer(VulkanDevice, mContext.framebuffers[i], nullptr);
		vkDestroyImageView(VulkanDevice, mContext.imageViews[i], nullptr);
	}
	mContext.framebuffers.clear();
	mContext.imageViews.clear();
	vkDestroyRenderPass(VulkanDevice, As<VkRenderPass>(mRenderPass), nullptr);
	vkDestroySwapchainKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), nullptr);

	// Destroy sync objects
	for (uint32_t i = 0; i < mProperties.maxFramesInFlight; i++)
	{
		vkDestroyFence(VulkanDevice, mContext.imageAcquireFences[i], nullptr);
		vkFreeCommandBuffers(VulkanDevice, mContext.commandPools[i], 1, &mContext.commandBuffers[i]);
		vkDestroyCommandPool(VulkanDevice, mContext.commandPools[i], nullptr);
		vkDestroySemaphore(VulkanDevice, mContext.imageAcquireSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanDevice, mContext.imageReleaseSemaphores[i], nullptr);
	}
	mContext.imageAcquireSemaphores.clear();
	mContext.imageReleaseSemaphores.clear();
	mContext.imageAcquireFences.clear();
	mContext.commandPools.clear();

	// Destroy surface
	vkDestroySurfaceKHR(As<VkInstance>(RendererContext::GetEntryPoint()), As<VkSurface>(mSurface), nullptr);
    
    // Destroy instance
#ifdef GDEBUG
    vkDestroyDebugUtilsMessengerEXT(mContext.instance, mContext.debugMessenger, nullptr);
#endif
    vkDestroyInstance(mContext.instance, nullptr);
}

const FrameObject& Swapchain::AcquireNextFrame()
{
	mContext.currentFrame.commandPool = mContext.commandPools[mCurrentFrameIndex];
	mContext.currentFrame.commandBuffer = mContext.commandBuffers[mCurrentFrameIndex];
	mContext.currentFrame.imageAcquireFence = mContext.imageAcquireFences[mCurrentFrameIndex];
	mContext.currentFrame.imageAcquireSemaphore = mContext.imageAcquireSemaphores[mCurrentFrameIndex];
	mContext.currentFrame.imageReleaseSemaphore = mContext.imageReleaseSemaphores[mCurrentFrameIndex];

	VK_CHECK(vkWaitForFences(VulkanDevice, 1, &mContext.currentFrame.imageAcquireFence, VK_TRUE, UINT64_MAX));

	VkResult result = vkAcquireNextImageKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), UINT64_MAX, mContext.currentFrame.imageAcquireSemaphore, VK_NULL_HANDLE, &mContext.currentFrame.imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		InvalidateAndCreate();
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Swapbuffers failed to acquire next image!");
	}
	mContext.currentFrame.swapchainImage = mContext.images[mContext.currentFrame.imageIndex];
	mContext.currentFrame.framebuffer = mContext.framebuffers[mContext.currentFrame.imageIndex];

	VK_CHECK(vkResetCommandPool(VulkanDevice, mContext.currentFrame.commandPool, 0));

	VkCommandBufferBeginInfo commandBufferBeginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mContext.currentFrame.commandBuffer, &commandBufferBeginInfo));

	return mContext.currentFrame;
}

void Swapchain::Present()
{
	VK_CHECK(vkEndCommandBuffer(mContext.currentFrame.commandBuffer));

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &mContext.currentFrame.imageAcquireSemaphore;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mContext.currentFrame.commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mContext.currentFrame.imageReleaseSemaphore;

	VK_CHECK(vkResetFences(VulkanDevice, 1, &mContext.currentFrame.imageAcquireFence));
	VK_CHECK(vkQueueSubmit(As<VkQueue>(RendererContext::GetGraphicsQueue()), 1, &submitInfo, mContext.currentFrame.imageAcquireFence));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mContext.currentFrame.imageReleaseSemaphore;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = As<VkSwapchainKHR*>(&mHandle);
	presentInfo.pImageIndices = &mContext.currentFrame.imageIndex;
	VkResult result = vkQueuePresentKHR(As<VkQueue>(RendererContext::GetGraphicsQueue()), &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		InvalidateAndCreate();
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Image presentation failed!");
	}

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mProperties.maxFramesInFlight;
}

NativeGraphicsHandle Swapchain::GetGraphicsCommandPool(uint32_t index) const
{
	return mContext.commandPools[index];
}

TextureFormat Swapchain::GetFormat() const
{
	return VkFormatToTextureFormat(mContext.imageFormat);
}

NativeGraphicsHandle GetLayer() const
{
    return mContext.instance;
}

const FrameObject& Swapchain::GetCurrentFrame() const
{
	return mContext.currentFrame;
}

void Swapchain::InvalidateAndCreate()
{
	SDL_Window* window = ApplicationInstance.GetActiveWindow().GetSDLWindow();

	vkDeviceWaitIdle(VulkanDevice);

	// Get surface information
	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurface>(mSurface), &presentModeCount, nullptr));
	mContext.presentModes.resize(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurface>(mSurface), &presentModeCount, mContext.presentModes.data()));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurface>(mSurface), &surfaceFormatCount, nullptr));
	mContext.surfaceFormats.resize(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurface>(mSurface), &surfaceFormatCount, mContext.surfaceFormats.data()));

	mContext.imageFormat = []()
	{
		for (const auto& surfaceFormat : mContext.surfaceFormats)
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

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurface>(mSurface), &surfaceCapabilities));

	if (surfaceCapabilities.minImageCount <= 3 && (surfaceCapabilities.maxImageCount >= 3 || surfaceCapabilities.maxImageCount == 0) && mProperties.tripleBufferingEnabled)
	{
		mProperties.maxFramesInFlight = 3;
		GLEAM_CORE_TRACE("Vulkan: Triple buffering enabled.");
	}
	else if (surfaceCapabilities.minImageCount <= 2 && (surfaceCapabilities.maxImageCount >= 2 || surfaceCapabilities.maxImageCount == 0))
	{
		mProperties.maxFramesInFlight = 2;
		if (mProperties.tripleBufferingEnabled)
		{
			mProperties.tripleBufferingEnabled = false;
		}
		GLEAM_CORE_TRACE("Vulkan: Double buffering enabled.");
	}
	else
	{
		mProperties.maxFramesInFlight = 1;
		GLEAM_ASSERT(false, "Vulkan: Neither triple nor double buffering is available!");
	}

	int width, height;
	SDL_Vulkan_GetDrawableSize(window, &width, &height);
	VkExtent2D imageExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	imageExtent.width = Math::Clamp(imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	imageExtent.height = Math::Clamp(imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
	mProperties.width = imageExtent.width;
	mProperties.height = imageExtent.height;

	// Create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = As<VkSurface>(mSurface);
	swapchainCreateInfo.minImageCount = mProperties.maxFramesInFlight;
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

		for (const auto& presentMode : mContext.presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentMode;
			}
		}
	#else
		mProperties.vsync = true;
	#endif
		return VK_PRESENT_MODE_FIFO_KHR;
	}();
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageFormat = mContext.imageFormat;
	swapchainCreateInfo.oldSwapchain = As<VkSwapchainKHR>(mHandle);
	VkSwapchainKHR newSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(VulkanDevice, &swapchainCreateInfo, nullptr, &newSwapchain));

	// Destroy swapchain
	if (As<VkSwapchainKHR>(mHandle) != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < mContext.framebuffers.size(); i++)
		{
			vkDestroyFramebuffer(VulkanDevice, mContext.framebuffers[i], nullptr);
			vkDestroyImageView(VulkanDevice, mContext.imageViews[i], nullptr);
		}
		mContext.framebuffers.clear();
		mContext.imageViews.clear();
		vkDestroyRenderPass(VulkanDevice, As<VkRenderPass>(mRenderPass), nullptr);
		vkDestroySwapchainKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), nullptr);
	}
	mHandle = newSwapchain;

	uint32_t swapchainImageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), &swapchainImageCount, nullptr));

	mContext.images.resize(swapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), &swapchainImageCount, mContext.images.data()));

	// Create image views
	mContext.imageViews.resize(swapchainImageCount);
	VkImageViewUsageCreateInfo imageViewUsageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
	imageViewUsageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.pNext = &imageViewUsageCreateInfo;
	imageViewCreateInfo.format = mContext.imageFormat;
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
		imageViewCreateInfo.image = mContext.images[i];
		VK_CHECK(vkCreateImageView(VulkanDevice, &imageViewCreateInfo, nullptr, &mContext.imageViews[i]));
	}

	// Create render pass
	VkAttachmentReference attachmentRef{};
	attachmentRef.attachment = 0;
	attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc{};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &attachmentRef;

	VkSubpassDependency subpassDependency{};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpassDependency.dependencyFlags = 0;

	VkAttachmentDescription attachmentDesc{};
	attachmentDesc.format = mContext.imageFormat;
	attachmentDesc.samples = GetVkSampleCount(mProperties.sampleCount);
	attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VK_CHECK(vkCreateRenderPass(VulkanDevice, &renderPassCreateInfo, nullptr, As<VkRenderPass*>(&mRenderPass)));
	mContext.currentFrame.swapchainRenderPass = As<VkRenderPass>(mRenderPass);

	// Create framebuffer
	mContext.framebuffers.resize(swapchainImageCount);

	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.width = mProperties.width;
	framebufferCreateInfo.height = mProperties.height;
	framebufferCreateInfo.renderPass = As<VkRenderPass>(mRenderPass);
	framebufferCreateInfo.attachmentCount = 1;
	framebufferCreateInfo.layers = 1;
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		framebufferCreateInfo.pAttachments = &mContext.imageViews[i];
		VK_CHECK(vkCreateFramebuffer(VulkanDevice, &framebufferCreateInfo, nullptr, &mContext.framebuffers[i]));
	}

	// Create sync objects
	static std::once_flag flag;
	std::call_once(flag, [this]()
	{
		mContext.imageAcquireSemaphores.resize(mProperties.maxFramesInFlight);
		mContext.imageReleaseSemaphores.resize(mProperties.maxFramesInFlight);
		mContext.imageAcquireFences.resize(mProperties.maxFramesInFlight);
		mContext.commandPools.resize(mProperties.maxFramesInFlight);
		mContext.commandBuffers.resize(mProperties.maxFramesInFlight);

		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

		VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		commandPoolCreateInfo.queueFamilyIndex = RendererContext::GetGraphicsQueueIndex();
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkCommandBufferAllocateInfo commandBufferAllocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferAllocateInfo.commandBufferCount = 1;
		commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < mProperties.maxFramesInFlight; i++)
		{
			VK_CHECK(vkCreateSemaphore(VulkanDevice, &semaphoreCreateInfo, nullptr, &mContext.imageAcquireSemaphores[i]));
			VK_CHECK(vkCreateSemaphore(VulkanDevice, &semaphoreCreateInfo, nullptr, &mContext.imageReleaseSemaphores[i]));

			VK_CHECK(vkCreateCommandPool(VulkanDevice, &commandPoolCreateInfo, nullptr, &mContext.commandPools[i]));
			commandBufferAllocateInfo.commandPool = mContext.commandPools[i];

			VK_CHECK(vkAllocateCommandBuffers(VulkanDevice, &commandBufferAllocateInfo, &mContext.commandBuffers[i]));

			VK_CHECK(vkCreateFence(VulkanDevice, &fenceCreateInfo, nullptr, &mContext.imageAcquireFences[i]));
		}
	});
}

#endif
