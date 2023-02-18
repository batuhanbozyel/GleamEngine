#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include "Core/Window.h"
#include "Core/Application.h"
#include "Core/Events/RendererEvent.h"
#include "Renderer/RendererConfig.h"

#include <SDL_vulkan.h>

using namespace Gleam;

void VulkanSwapchain::Initialize(const RendererConfig& config)
{
	SDL_Window* window = ApplicationInstance.GetActiveWindow().GetSDLWindow();
	mSampleCount = config.sampleCount;

    // Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(window, VulkanDevice::GetInstance(), &mSurface);
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan: Surface creation failed!");

	// Get surface information
	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice::GetPhysicalDevice(), mSurface, &presentModeCount, nullptr));
	TArray<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(VulkanDevice::GetPhysicalDevice(), mSurface, &presentModeCount, presentModes.data()));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice::GetPhysicalDevice(), mSurface, &surfaceFormatCount, nullptr));
	mSurfaceFormats.resize(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(VulkanDevice::GetPhysicalDevice(), mSurface, &surfaceFormatCount, mSurfaceFormats.data()));

	mImageFormat = [&]()
	{
		for (const auto& surfaceFormat : mSurfaceFormats)
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
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VulkanDevice::GetPhysicalDevice(), mSurface, &surfaceCapabilities));

	if (surfaceCapabilities.minImageCount <= 3 && (surfaceCapabilities.maxImageCount >= 3 || surfaceCapabilities.maxImageCount == 0) && config.tripleBufferingEnabled)
	{
		mMaxFramesInFlight = 3;
		GLEAM_CORE_TRACE("Vulkan: Triple buffering enabled.");
	}
	else if (surfaceCapabilities.minImageCount <= 2 && (surfaceCapabilities.maxImageCount >= 2 || surfaceCapabilities.maxImageCount == 0))
	{
		mMaxFramesInFlight = 2;
		GLEAM_CORE_TRACE("Vulkan: Double buffering enabled.");
	}
	else
	{
		mMaxFramesInFlight = 1;
		GLEAM_ASSERT(false, "Vulkan: Neither triple nor double buffering is available!");
	}
	mMinImageExtent = surfaceCapabilities.minImageExtent;
	mMaxImageExtent = surfaceCapabilities.maxImageExtent;

	mPresentMode = [&]()
	{
	#ifndef PLATFORM_ANDROID // use FIFO for mobile to reduce power consumption
		if (!config.vsync)
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
}

void VulkanSwapchain::Destroy()
{
	// Destroy swapchain
	vkDestroySwapchainKHR(VulkanDevice::GetHandle(), mHandle, nullptr);

	// Destroy drawable
	for (uint32_t i = 0; i < mImages.size(); i++)
	{
		vkDestroyImageView(VulkanDevice::GetHandle(), mImages[i].view, nullptr);
	}

	// Destroy sync objects
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		vkDestroySemaphore(VulkanDevice::GetHandle(), mImageAcquireSemaphores[i], nullptr);
		vkDestroySemaphore(VulkanDevice::GetHandle(), mImageReleaseSemaphores[i], nullptr);
	}
	mImageAcquireSemaphores.clear();
	mImageReleaseSemaphores.clear();
}

const VulkanDrawable& VulkanSwapchain::AcquireNextDrawable()
{
	VkResult result = vkAcquireNextImageKHR(VulkanDevice::GetHandle(), mHandle, UINT64_MAX, mImageAcquireSemaphores[mCurrentFrameIndex], VK_NULL_HANDLE, &mImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		InvalidateAndCreate();
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Swapbuffers failed to acquire next image!");
	}

	return mImages[mImageIndex];
}

void VulkanSwapchain::Present()
{
	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mImageReleaseSemaphores[mCurrentFrameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mHandle;
	presentInfo.pImageIndices = &mImageIndex;
	VkResult result = vkQueuePresentKHR(VulkanDevice::GetGraphicsQueue().handle, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		InvalidateAndCreate();
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Image presentation failed!");
	}

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
}

void VulkanSwapchain::InvalidateAndCreate()
{
	vkDeviceWaitIdle(VulkanDevice::GetHandle());
	
	int width, height;
	SDL_Vulkan_GetDrawableSize(ApplicationInstance.GetActiveWindow().GetSDLWindow(), &width, &height);
	VkExtent2D imageExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	imageExtent.width = Math::Clamp(imageExtent.width, mMinImageExtent.width, mMaxImageExtent.width);
	imageExtent.height = Math::Clamp(imageExtent.height, mMinImageExtent.height, mMaxImageExtent.height);
	mSize.width = static_cast<float>(imageExtent.width);
	mSize.height = static_cast<float>(imageExtent.height);

	// Create swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
	swapchainCreateInfo.surface = mSurface;
	swapchainCreateInfo.minImageCount = mMaxFramesInFlight;
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
	swapchainCreateInfo.presentMode = mPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageFormat = mImageFormat;
	swapchainCreateInfo.oldSwapchain = mHandle;
	VkSwapchainKHR newSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(VulkanDevice::GetHandle(), &swapchainCreateInfo, nullptr, &newSwapchain));

	// Destroy swapchain
	if (mHandle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(VulkanDevice::GetHandle(), mHandle, nullptr);
	}
	mHandle = newSwapchain;

	// Destroy drawable
	for (uint32_t i = 0; i < mImages.size(); i++)
	{
		vkDestroyImageView(VulkanDevice::GetHandle(), mImages[i].view, nullptr);
	}
	mImages.clear();

	uint32_t swapchainImageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice::GetHandle(), mHandle, &swapchainImageCount, nullptr));

	TArray<VkImage> drawables(swapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(VulkanDevice::GetHandle(), mHandle, &swapchainImageCount, drawables.data()));

	// Create image views
	VkImageViewUsageCreateInfo imageViewUsageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
	imageViewUsageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	imageViewCreateInfo.pNext = &imageViewUsageCreateInfo;
	imageViewCreateInfo.format = mImageFormat;
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

	mImages.resize(swapchainImageCount);
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		mImages[i].image = drawables[i];
		imageViewCreateInfo.image = drawables[i];
		VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &imageViewCreateInfo, nullptr, &mImages[i].view));
	}

	// Create sync objects
	static std::once_flag flag;
	std::call_once(flag, [this]()
	{
		mImageAcquireSemaphores.resize(mMaxFramesInFlight);
		mImageReleaseSemaphores.resize(mMaxFramesInFlight);
		
		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
		{
			VK_CHECK(vkCreateSemaphore(VulkanDevice::GetHandle(), &semaphoreCreateInfo, nullptr, &mImageAcquireSemaphores[i]));
			VK_CHECK(vkCreateSemaphore(VulkanDevice::GetHandle(), &semaphoreCreateInfo, nullptr, &mImageReleaseSemaphores[i]));
		}
	});

	EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(mSize));
}

const VulkanDrawable& VulkanSwapchain::GetDrawable() const
{
	return mImages[mImageIndex];
}

TextureFormat VulkanSwapchain::GetFormat() const
{
	return VkFormatToTextureFormat(mImageFormat);
}

VkSwapchainKHR VulkanSwapchain::GetHandle() const
{
	return mHandle;
}

VkSurfaceKHR VulkanSwapchain::GetSurface() const
{
	return mSurface;
}

VkSemaphore VulkanSwapchain::GetImageAcquireSemaphore() const
{
	return mImageAcquireSemaphores[mCurrentFrameIndex];
}

VkSemaphore VulkanSwapchain::GetImageReleaseSemaphore() const
{
	return mImageReleaseSemaphores[mCurrentFrameIndex];
}

const Size& VulkanSwapchain::GetSize() const
{
	return mSize;
}

uint32_t VulkanSwapchain::GetSampleCount() const
{
	return mSampleCount;
}

uint32_t VulkanSwapchain::GetFrameIndex() const
{
	return mCurrentFrameIndex;
}

uint32_t VulkanSwapchain::GetMaxFramesInFlight() const
{
	return mMaxFramesInFlight;
}

#endif
