#pragma once
#ifdef USE_VULKAN_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <volk.h>
#include <vk_mem_alloc.h>

namespace Gleam {

struct VulkanQueue
{
	VkQueue handle{ VK_NULL_HANDLE };
	uint32_t index{ 0 };
};

class VulkanDevice final : public GraphicsDevice
{
public:

    VulkanDevice();

    ~VulkanDevice();

	VkCommandBuffer AllocateCommandBuffer();

	VkFence CreateFence();

	VkRenderPass CreateRenderPass(const VkRenderPassCreateInfo& createInfo);

	VkFramebuffer CreateFramebuffer(const VkFramebufferCreateInfo& createInfo);

	const VulkanQueue& GetGraphicsQueue() const;

	const VulkanQueue& GetComputeQueue() const;

	const VulkanQueue& GetTransferQueue() const;

	VkInstance GetInstance() const;

	VkPhysicalDevice GetPhysicalDevice() const;

	VkPipelineCache GetPipelineCache() const;

	VmaAllocator GetAllocator() const;

private:

	// Instance
	VkInstance mInstance{ VK_NULL_HANDLE };
	TArray<VkExtensionProperties> mExtensions;

	// Debug/Validation layer
#ifdef GDEBUG
	VkDebugUtilsMessengerEXT mDebugMessenger{ VK_NULL_HANDLE };
#endif

	// Device
	VkPhysicalDevice mPhysicalDevice{ VK_NULL_HANDLE };

	// Queue
	VulkanQueue mGraphicsQueue;
	VulkanQueue mComputeQueue;
	VulkanQueue mTransferQueue;

	TArray<VkExtensionProperties> mDeviceExtensions;
	VkPhysicalDeviceMemoryProperties mMemoryProperties;

	// Pipeline cache
	VkPipelineCache mPipelineCache{ VK_NULL_HANDLE };

	// VMA
	VmaAllocator mAllocator;
};

} // namespace Gleam
#endif
