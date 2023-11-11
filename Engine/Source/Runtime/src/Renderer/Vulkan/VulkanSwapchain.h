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
	void Present();

	VkCommandBuffer AllocateCommandBuffer();

	VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& createInfo);

	VkFramebuffer CreateFramebuffer(const VkFramebufferCreateInfo& createInfo);

	VkSemaphore GetImageAcquireSemaphore() const;

	VkSemaphore GetImageReleaseSemaphore() const;

	VkCommandPool GetCommandPool(uint32_t frameIdx) const;

	VkCommandPool GetCommandPool() const;

	VkSurfaceKHR GetSurface() const;

	virtual void Flush(uint32_t frameIndex) override;

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
	};
	TArray<FramePool> mFrameObjects;
	TArray<VulkanDrawable> mImages;
	uint32_t mImageIndex = 0;

	// Command Pool
	TArray<VkCommandPool> mCommandPools;

	// Synchronization
	TArray<VkSemaphore> mImageAcquireSemaphores;
	TArray<VkSemaphore> mImageReleaseSemaphores;

	VkSwapchainKHR mHandle{ VK_NULL_HANDLE };

	VulkanDevice* mDevice = nullptr;

};

} // namespace Gleam
#endif
