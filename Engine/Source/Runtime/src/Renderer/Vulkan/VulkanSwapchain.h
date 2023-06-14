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

	void Initialize(const RendererConfig& config);

	void Destroy();

	void InvalidateAndCreate();

	const VulkanDrawable& AcquireNextDrawable();
	void Present();

	const VulkanDrawable& GetDrawable() const;

    TextureFormat GetFormat() const;

	VkSwapchainKHR GetHandle() const;

	VkSurfaceKHR GetSurface() const;

	VkSemaphore GetImageAcquireSemaphore() const;

	VkSemaphore GetImageReleaseSemaphore() const;
    
    const Size& GetSize() const;

	uint32_t GetSampleCount() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetFramesInFlight() const;

private:
    
	// Surface
	VkSurfaceKHR mSurface{ VK_NULL_HANDLE };
	TArray<VkSurfaceFormatKHR> mSurfaceFormats;
	VkPresentModeKHR mPresentMode;
	VkExtent2D mMinImageExtent;
	VkExtent2D mMaxImageExtent;

	// Frame
	TArray<VulkanDrawable> mImages;
	VkFormat mImageFormat;
	uint32_t mImageIndex;
	Size mSize = Size::zero;
	uint32_t mSampleCount = 1;

	// Synchronization
	TArray<VkSemaphore> mImageAcquireSemaphores;
	TArray<VkSemaphore> mImageReleaseSemaphores;
	uint32_t mMaxFramesInFlight = 3;
	uint32_t mCurrentFrameIndex = 0;

	VkSwapchainKHR mHandle{ VK_NULL_HANDLE };

};

} // namespace Gleam
#endif
