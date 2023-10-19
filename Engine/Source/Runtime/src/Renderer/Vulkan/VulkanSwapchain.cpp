#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

#include "Core/WindowSystem.h"
#include "Core/Application.h"
#include "Core/Events/RendererEvent.h"
#include "Renderer/RenderSystem.h"

#include <SDL_vulkan.h>

using namespace Gleam;

void VulkanSwapchain::Initialize(VulkanDevice* device)
{
	mDevice = device;
	auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();

    // Create surface
	bool surfaceCreateResult = SDL_Vulkan_CreateSurface(windowSystem->GetSDLWindow(), device->GetInstance(), &mSurface);
	GLEAM_ASSERT(surfaceCreateResult, "Vulkan: Surface creation failed!");
}

void VulkanSwapchain::Destroy()
{
	for (auto& pool : mPooledObjects)
	{
		for (auto& [obj, deallocator] : pool)
		{
			deallocator(obj);
		}
	}
	mPooledObjects.clear();

	// Destroy swapchain
	vkDestroySwapchainKHR(As<VkDevice>(mDevice->GetHandle()), mHandle, nullptr);

	// Destroy surface
	vkDestroySurfaceKHR(mDevice->GetInstance(), mSurface, nullptr);

	// Destroy drawable
	for (uint32_t i = 0; i < mImages.size(); i++)
	{
		vkDestroyImageView(As<VkDevice>(mDevice->GetHandle()), mImages[i].view, nullptr);
	}

	// Destroy command pools
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		vkDestroyCommandPool(As<VkDevice>(mDevice->GetHandle()), mCommandPools[i], nullptr);
	}
	mCommandPools.clear();

	// Destroy sync objects
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		vkDestroySemaphore(As<VkDevice>(mDevice->GetHandle()), mImageAcquireSemaphores[i], nullptr);
		vkDestroySemaphore(As<VkDevice>(mDevice->GetHandle()), mImageReleaseSemaphores[i], nullptr);
		vkDestroyFence(As<VkDevice>(mDevice->GetHandle()), mFences[i], nullptr);
	}
	mImageAcquireSemaphores.clear();
	mImageReleaseSemaphores.clear();
	mFences.clear();
}

const VulkanDrawable& VulkanSwapchain::AcquireNextDrawable()
{
	VkResult result = vkAcquireNextImageKHR(As<VkDevice>(mDevice->GetHandle()), mHandle, UINT64_MAX, mImageAcquireSemaphores[mCurrentFrameIndex], VK_NULL_HANDLE, &mImageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		auto renderSystem = GameInstance->GetSubsystem<RenderSystem>();
		Configure(renderSystem->GetConfiguration());
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Swapbuffers failed to acquire next image!");
	}

	return mImages[mImageIndex];
}

void VulkanSwapchain::Present(VkCommandBuffer commandBuffer)
{
    uint32_t frameIndex = mCurrentFrameIndex;
    
	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &mImageAcquireSemaphores[mCurrentFrameIndex];
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &mImageReleaseSemaphores[mCurrentFrameIndex];
	VK_CHECK(vkQueueSubmit(mDevice->GetGraphicsQueue().handle, 1, &submitInfo, mFences[mCurrentFrameIndex]));

	VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = &mImageReleaseSemaphores[mCurrentFrameIndex];
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &mHandle;
	presentInfo.pImageIndices = &mImageIndex;
	VkResult result = vkQueuePresentKHR(mDevice->GetGraphicsQueue().handle, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		auto renderSystem = GameInstance->GetSubsystem<RenderSystem>();
		Configure(renderSystem->GetConfiguration());
	}
	else
	{
		GLEAM_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Vulkan: Image presentation failed!");
	}

	mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;

	VK_CHECK(vkWaitForFences(As<VkDevice>(mDevice->GetHandle()), 1, &mFences[mCurrentFrameIndex], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetCommandPool(As<VkDevice>(mDevice->GetHandle()), mCommandPools[mCurrentFrameIndex], 0));
	VK_CHECK(vkResetFences(As<VkDevice>(mDevice->GetHandle()), 1, &mFences[mCurrentFrameIndex]));
	
	auto& pooledObjects = mPooledObjects[mCurrentFrameIndex];
	for (auto& [obj, deallocate] : pooledObjects)
	{
		deallocate(obj);
	}
	pooledObjects.clear();
}

void VulkanSwapchain::Configure(const RendererConfig& config)
{
	auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();
	vkDeviceWaitIdle(As<VkDevice>(mDevice->GetHandle()));

	// Get surface information
	uint32_t presentModeCount;
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->GetPhysicalDevice(), mSurface, &presentModeCount, nullptr));
	TArray<VkPresentModeKHR> presentModes(presentModeCount);
	VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->GetPhysicalDevice(), mSurface, &presentModeCount, presentModes.data()));

	uint32_t surfaceFormatCount;
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->GetPhysicalDevice(), mSurface, &surfaceFormatCount, nullptr));
	TArray<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
	VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->GetPhysicalDevice(), mSurface, &surfaceFormatCount, surfaceFormats.data()));

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice->GetPhysicalDevice(), mSurface, &surfaceCapabilities));

	auto imageFormat = [&]()
	{
		for (const auto& surfaceFormat : surfaceFormats)
		{
			if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
			{
				return VK_FORMAT_B8G8R8A8_UNORM;
			}
		}
		return VK_FORMAT_R8G8B8A8_UNORM;
	}();
	mFormat = VkFormatToTextureFormat(imageFormat);

	// Configure triple buffering
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

	VkPresentModeKHR presentMode = [&]()
	{
	#ifndef PLATFORM_ANDROID // use FIFO for mobile to reduce power consumption
		if (!config.vsync)
		{
			return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}

		for (auto presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return presentMode;
			}
		}
	#endif
		return VK_PRESENT_MODE_FIFO_KHR;
	}();

	// Get swapchain extent
	int width, height;
	SDL_GetWindowSizeInPixels(windowSystem->GetSDLWindow(), &width, &height);
	VkExtent2D imageExtent{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
	imageExtent.width = Math::Clamp(imageExtent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
	imageExtent.height = Math::Clamp(imageExtent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);
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
	swapchainCreateInfo.presentMode = presentMode;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageFormat = imageFormat;
	swapchainCreateInfo.oldSwapchain = mHandle;
	VkSwapchainKHR newSwapchain;
	VK_CHECK(vkCreateSwapchainKHR(As<VkDevice>(mDevice->GetHandle()), &swapchainCreateInfo, nullptr, &newSwapchain));

	// Destroy swapchain
	if (mHandle != VK_NULL_HANDLE)
	{
		vkDestroySwapchainKHR(As<VkDevice>(mDevice->GetHandle()), mHandle, nullptr);

		// Flush pooled objects
		for (auto& pool : mPooledObjects)
		{
			for (auto& [obj, deallocator] : pool)
			{
				deallocator(obj);
			}
		}
		mPooledObjects.clear();

		// Destroy command pools
		for (uint32_t i = 0; i < mCommandPools.size(); i++)
		{
			vkDestroyCommandPool(As<VkDevice>(mDevice->GetHandle()), mCommandPools[i], nullptr);
		}
		mCommandPools.clear();

		// Destroy sync objects
		for (uint32_t i = 0; i < mImageAcquireSemaphores.size(); i++)
		{
			vkDestroySemaphore(As<VkDevice>(mDevice->GetHandle()), mImageAcquireSemaphores[i], nullptr);
			vkDestroySemaphore(As<VkDevice>(mDevice->GetHandle()), mImageReleaseSemaphores[i], nullptr);
			vkDestroyFence(As<VkDevice>(mDevice->GetHandle()), mFences[i], nullptr);
		}
		mImageAcquireSemaphores.clear();
		mImageReleaseSemaphores.clear();
		mFences.clear();

		// Destroy drawable
		for (uint32_t i = 0; i < mImages.size(); i++)
		{
			vkDestroyImageView(As<VkDevice>(mDevice->GetHandle()), mImages[i].view, nullptr);
		}
		mImages.clear();
	}
	mHandle = newSwapchain;

	uint32_t swapchainImageCount;
	VK_CHECK(vkGetSwapchainImagesKHR(As<VkDevice>(mDevice->GetHandle()), mHandle, &swapchainImageCount, nullptr));

	TArray<VkImage> drawables(swapchainImageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(As<VkDevice>(mDevice->GetHandle()), mHandle, &swapchainImageCount, drawables.data()));

	// Create image views
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

	mImages.resize(swapchainImageCount);
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		mImages[i].image = drawables[i];
		imageViewCreateInfo.image = drawables[i];
		VK_CHECK(vkCreateImageView(As<VkDevice>(mDevice->GetHandle()), &imageViewCreateInfo, nullptr, &mImages[i].view));
	}

	// Create sync objects
	mImageAcquireSemaphores.resize(mMaxFramesInFlight);
	mImageReleaseSemaphores.resize(mMaxFramesInFlight);
	mFences.resize(mMaxFramesInFlight);

	VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		VK_CHECK(vkCreateSemaphore(As<VkDevice>(mDevice->GetHandle()), &semaphoreCreateInfo, nullptr, &mImageAcquireSemaphores[i]));
		VK_CHECK(vkCreateSemaphore(As<VkDevice>(mDevice->GetHandle()), &semaphoreCreateInfo, nullptr, &mImageReleaseSemaphores[i]));

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		if (i >= 1) { fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; }
		VK_CHECK(vkCreateFence(As<VkDevice>(mDevice->GetHandle()), &fenceCreateInfo, nullptr, &mFences[i]));
	}

	// Create command pools
	VkCommandPoolCreateInfo commandPoolCreateInfo{ VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	mCommandPools.resize(mMaxFramesInFlight);
	mPooledObjects.resize(mMaxFramesInFlight);
	for (uint32_t i = 0; i < mMaxFramesInFlight; i++)
	{
		commandPoolCreateInfo.queueFamilyIndex = mDevice->GetGraphicsQueue().index;
		VK_CHECK(vkCreateCommandPool(As<VkDevice>(mDevice->GetHandle()), &commandPoolCreateInfo, nullptr, &mCommandPools[i]));
	}

	EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(mSize));
}

const VulkanDrawable& VulkanSwapchain::GetDrawable() const
{
	return mImages[mImageIndex];
}

VkCommandPool VulkanSwapchain::GetCommandPool() const
{
	return mCommandPools[mCurrentFrameIndex];
}

VkCommandPool VulkanSwapchain::GetCommandPool(uint32_t frameIdx) const
{
	return mCommandPools[frameIdx];
}

VkFence VulkanSwapchain::GetFence() const
{
	return mFences[mCurrentFrameIndex];
}

VkSurfaceKHR VulkanSwapchain::GetSurface() const
{
	return mSurface;
}

#endif
