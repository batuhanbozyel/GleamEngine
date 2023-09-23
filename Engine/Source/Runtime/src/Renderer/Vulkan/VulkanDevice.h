#pragma once
#ifdef USE_VULKAN_RENDERER
#include "VulkanSwapchain.h"

namespace Gleam {

class VulkanDevice final
{
public:

    static void Init();

    static void Destroy();

	static const VulkanQueue& GetGraphicsQueue();

	static const VulkanQueue& GetComputeQueue();

	static const VulkanQueue& GetTransferQueue();

	static VulkanSwapchain& GetSwapchain();

	static VkDevice GetHandle();

	static VkInstance GetInstance();

	static VkPhysicalDevice GetPhysicalDevice();

	static VkPipelineCache GetPipelineCache();

	static uint32_t GetMemoryTypeForProperties(uint32_t memoryTypeBits, uint32_t properties);

private:

	// Device
	static inline VkDevice mHandle{ VK_NULL_HANDLE };

	// Instance
	static inline VkInstance mInstance{ VK_NULL_HANDLE };
	static inline TArray<VkExtensionProperties> mExtensions;

	// Debug/Validation layer
#ifdef GDEBUG
	static inline VkDebugUtilsMessengerEXT mDebugMessenger{ VK_NULL_HANDLE };
#endif

	// Device
	static inline VkPhysicalDevice mPhysicalDevice{ VK_NULL_HANDLE };

	// Queue
	static inline VulkanQueue mGraphicsQueue;
	static inline VulkanQueue mComputeQueue;
	static inline VulkanQueue mTransferQueue;

	static inline TArray<VkExtensionProperties> mDeviceExtensions;
	static inline VkPhysicalDeviceMemoryProperties mMemoryProperties;

	// Pipeline cache
	static inline VkPipelineCache mPipelineCache{ VK_NULL_HANDLE };
	
	// Swapchain
	static inline VulkanSwapchain mSwapchain;
};

} // namespace Gleam
#endif
