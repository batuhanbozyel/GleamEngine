#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanDevice.h"
#include "VulkanUtils.h"

namespace Gleam {

class VulkanTransitionManager
{
public:

	static void Init(VulkanDevice* device);

	static void TransitionLayout(VkCommandBuffer cmd, VkImage image, VkImageLayout layout);

	static void SetLayout(VkImage image, VkImageLayout layout);

	static VkImageLayout GetLayout(VkImage image);

	static void Clear();

private:

	static inline VulkanDevice* mDevice = nullptr;

	static inline HashMap<VkImage, VkImageLayout> mImageLayoutCache;

};

} // namespace Gleam
#endif
