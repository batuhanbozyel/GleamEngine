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

	const VulkanDrawable& GetDrawable() const;

	VkCommandPool GetCommandPool(uint32_t frameIdx) const;

	VkCommandPool GetCommandPool() const;

	VkFence GetFence() const;

	VkSurfaceKHR GetSurface() const;

private:

	// Surface
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };

	// Frame
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
