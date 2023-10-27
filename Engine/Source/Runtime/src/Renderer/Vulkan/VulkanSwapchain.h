#pragma once
#ifdef USE_VULKAN_RENDERER
#include "Renderer/Swapchain.h"

#include <volk.h>

namespace Gleam {

class VulkanDevice;
struct RendererConfig;
enum class TextureFormat;

struct VulkanDrawable
{
	VkImage image{ VK_NULL_HANDLE };
	VkImageView view{ VK_NULL_HANDLE };
};

class VulkanSwapchain final : public Swapchain
{
public:

	void Initialize(VulkanDevice* device);

	void Destroy();

	void Configure(const RendererConfig& config);

	const VulkanDrawable& AcquireNextDrawable();
	void Present(VkCommandBuffer commandBuffer);

	void AddFrameObject(VkCommandBuffer cmd)
	{
		mFrameObjects[mCurrentFrameIndex].commandBuffers.push_back(cmd);
	}

	void AddFrameObject(VkFramebuffer framebuffer)
	{
		mFrameObjects[mCurrentFrameIndex].framebuffers.push_back(framebuffer);
	}

	void AddFrameObject(VkRenderPass renderPass)
	{
		mFrameObjects[mCurrentFrameIndex].renderPasses.push_back(renderPass);
	}

	void AddFrameObject(VkFence fence)
	{
		mFrameObjects[mCurrentFrameIndex].fences.push_back(fence);
	}

	VkCommandPool GetCommandPool(uint32_t frameIdx) const;

	VkCommandPool GetCommandPool() const;

	VkSurfaceKHR GetSurface() const;

private:

	void FlushFrameObjects(uint32_t frameIndex);

	// Surface
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };

	// Frame
	struct FramePool
	{
		TArray<VkCommandBuffer> commandBuffers;
		TArray<VkFramebuffer> framebuffers;
		TArray<VkRenderPass> renderPasses;
		TArray<VkFence> fences;
	};
	TArray<FramePool> mFrameObjects;
	TArray<VulkanDrawable> mImages;
	uint32_t mImageIndex = 0;

	// Command Pool
	TArray<VkCommandPool> mCommandPools;

	// Synchronization
	TArray<VkSemaphore> mImageAcquireSemaphores;
	TArray<VkSemaphore> mImageReleaseSemaphores;
	TArray<VkFence> mFences;

	VkSwapchainKHR mHandle{ VK_NULL_HANDLE };

	VulkanDevice* mDevice = nullptr;

};

} // namespace Gleam
#endif
