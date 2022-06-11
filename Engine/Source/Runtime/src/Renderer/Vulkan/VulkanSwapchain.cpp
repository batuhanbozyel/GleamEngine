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
	// Surface
	TArray<VkSurfaceFormatKHR> surfaceFormats;
	TArray<VkPresentModeKHR> presentModes;

	// Drawable
	TArray<VkImage> images;
	TArray<VkImageView> imageViews;
	VkFormat imageFormat;
	uint32_t imageIndex;

	// Synchronization
	TArray<VkSemaphore> imageAcquireSemaphores;
	TArray<VkSemaphore> imageReleaseSemaphores;
	TArray<VkFence> imageAcquireFences;
} mContext;

Swapchain::Swapchain(const RendererProperties& props, NativeGraphicsHandle instance)
	: mProperties(props)
{
    // Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(ApplicationInstance.GetActiveWindow().GetSDLWindow(), As<VkInstance>(instance), As<VkSurfaceKHR*>(&mSurface));
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan: Surface creation failed!");
}

Swapchain::~Swapchain()
{
	// Destroy swapchain
	vkDestroySwapchainKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), nullptr);

	// Destroy drawable
	for (uint32_t i = 0; i < mContext.imageViews.size(); i++)
	{
		vkDestroyImageView(VulkanDevice, mContext.imageViews[i], nullptr);
	}

	// Destroy sync objects
	for (uint32_t i = 0; i < mProperties.maxFramesInFlight; i++)
	{
		vkDestroyFence(VulkanDevice, mContext.imageAcquireFences[i], nullptr);
		vkDestroySemaphore(VulkanDevice, mContext.imageAcquireSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanDevice, mContext.imageReleaseSemaphores[i], nullptr);
	}
	mContext.imageAcquireSemaphores.clear();
	mContext.imageReleaseSemaphores.clear();
	mContext.imageAcquireFences.clear();
}

NativeGraphicsHandle Swapchain::AcquireNextDrawable()
{
	VkResult result = vkAcquireNextImageKHR(VulkanDevice, As<VkSwapchainKHR>(mHandle), UINT64_MAX, mContext.imageAcquireSemaphores[mCurrentFrameIndex], VK_NULL_HANDLE, &mContext.imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		InvalidateAndCreate();
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Swapbuffers failed to acquire next image!");
	}

	return mContext.imageViews[mContext.imageIndex];
}

void Swapchain::Present(NativeGraphicsHandle commandBuffer)
{
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &mContext.imageAcquireSemaphores[mCurrentFrameIndex];
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = As<VkCommandBuffer*>(&commandBuffer);
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mContext.imageReleaseSemaphores[mCurrentFrameIndex];
	VK_CHECK(vkQueueSubmit(As<VkQueue>(RendererContext::GetGraphicsQueue()), 1, &submitInfo, mContext.imageAcquireFences[mCurrentFrameIndex]));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mContext.imageReleaseSemaphores[mCurrentFrameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = As<VkSwapchainKHR*>(&mHandle);
	presentInfo.pImageIndices = &mContext.imageIndex;
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

	VK_CHECK(vkWaitForFences(VulkanDevice, 1, &mContext.imageAcquireFences[mCurrentFrameIndex], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(VulkanDevice, 1, &mContext.imageAcquireFences[mCurrentFrameIndex]));
	VK_CHECK(vkResetCommandPool(VulkanDevice, As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(mCurrentFrameIndex)), 0));
}

TextureFormat Swapchain::GetFormat() const
{
	return VkFormatToTextureFormat(mContext.imageFormat);
}

NativeGraphicsHandle Swapchain::GetCurrentDrawable() const
{
	return mContext.imageViews[mContext.imageIndex];
}

void Swapchain::InvalidateAndCreate()
{
	SDL_Window* window = ApplicationInstance.GetActiveWindow().GetSDLWindow();

	vkDeviceWaitIdle(VulkanDevice);

	// Get surface information
	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurfaceKHR>(mSurface), &presentModeCount, nullptr));
	mContext.presentModes.resize(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurfaceKHR>(mSurface), &presentModeCount, mContext.presentModes.data()));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurfaceKHR>(mSurface), &surfaceFormatCount, nullptr));
	mContext.surfaceFormats.resize(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurfaceKHR>(mSurface), &surfaceFormatCount, mContext.surfaceFormats.data()));

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
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), As<VkSurfaceKHR>(mSurface), &surfaceCapabilities));

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
	swapchainCreateInfo.surface = As<VkSurfaceKHR>(mSurface);
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

	// Create sync objects
	static std::once_flag flag;
	std::call_once(flag, [this]()
	{
		mContext.imageAcquireSemaphores.resize(mProperties.maxFramesInFlight);
		mContext.imageReleaseSemaphores.resize(mProperties.maxFramesInFlight);
		mContext.imageAcquireFences.resize(mProperties.maxFramesInFlight);
		
		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < mProperties.maxFramesInFlight; i++)
		{
			VK_CHECK(vkCreateSemaphore(VulkanDevice, &semaphoreCreateInfo, nullptr, &mContext.imageAcquireSemaphores[i]));
			VK_CHECK(vkCreateSemaphore(VulkanDevice, &semaphoreCreateInfo, nullptr, &mContext.imageReleaseSemaphores[i]));
			VK_CHECK(vkCreateFence(VulkanDevice, &fenceCreateInfo, nullptr, &mContext.imageAcquireFences[i]));
		}
		VK_CHECK(vkResetFences(VulkanDevice, 1, &mContext.imageAcquireFences[mCurrentFrameIndex]));
	});
}

#endif
