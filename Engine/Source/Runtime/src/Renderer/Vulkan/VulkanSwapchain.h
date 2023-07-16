#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanUtils.h"

namespace Gleam {

struct Version;
struct RendererConfig;
enum class TextureFormat;

class VulkanSwapchain final
{
public:

	void Initialize();

	void Destroy();

	void Configure(const RendererConfig& config);

	const VulkanDrawable& AcquireNextDrawable();
	void Present(VkCommandBuffer commandBuffer);

	const VulkanDrawable& GetDrawable() const;

	VkCommandPool GetCommandPool() const;

	VkFence GetFence() const;

    TextureFormat GetFormat() const;

	VkSwapchainKHR GetHandle() const;

	VkSurfaceKHR GetSurface() const;
    
    const Size& GetSize() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetFramesInFlight() const;

private:

	// Surface
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };

	// Frame
	TArray<VulkanDrawable> mImages;
	VkFormat mImageFormat;
	uint32_t mImageIndex;
	Size mSize = Size::zero;

	// Command Pool
	TArray<VkCommandPool> mCommandPools;

	// Synchronization
	TArray<VkSemaphore> mImageAcquireSemaphores;
	TArray<VkSemaphore> mImageReleaseSemaphores;
	TArray<VkFence> mFences;

	uint32_t mMaxFramesInFlight = 3;
	uint32_t mCurrentFrameIndex = 0;

	VkSwapchainKHR mHandle{ VK_NULL_HANDLE };

};

} // namespace Gleam
#endif
