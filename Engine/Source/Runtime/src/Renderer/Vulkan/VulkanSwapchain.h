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

	VkCommandBuffer AllocateCommandBuffer();

	VkFence CreateFence();

	VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& createInfo);

	VkFramebuffer CreateFramebuffer(const VkFramebufferCreateInfo& createInfo);

	const VulkanDrawable& GetDrawable() const;

	VkCommandPool GetCommandPool(uint32_t frameIdx) const;

	VkCommandPool GetCommandPool() const;

	VkFence GetFence() const;

    TextureFormat GetFormat() const;

	VkSwapchainKHR GetHandle() const;

	VkSurfaceKHR GetSurface() const;
    
    const Size& GetSize() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetFramesInFlight() const;

private:

	struct ObjectPool
	{
		uint32_t frameIdx;
		TArray<VkRenderPass> renderPasses;
		TArray<VkFramebuffer> framebuffers;
		TArray<VkCommandBuffer> commandBuffers;
		TArray<VkFence> fences;

		void Flush();
	};
	TArray<ObjectPool> mFrameObjects;

	// Surface
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };

	// Frame
	TArray<VulkanDrawable> mImages;
	VkFormat mImageFormat = VK_FORMAT_UNDEFINED;
	uint32_t mImageIndex = 0;
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
