#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Buffer.h"
#include "VulkanDevice.h"

using namespace Gleam;

Buffer::Buffer(const BufferDescriptor& descriptor)
    : mDescriptor(descriptor)
{
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = BufferUsageToVkBufferUsage(descriptor.usage);

	if (descriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkBuffer*>(&mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;

	switch (descriptor.memoryType)
	{
		case MemoryType::GPU:
		{
			allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			break;
		}
		case MemoryType::Shared:
		{
			allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		}
		case MemoryType::CPU:
		{
			allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		}
		default:
		{
			GLEAM_ASSERT(false, "Vulkan: Unknown memory type given!");
			break;
		}
	}

	VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
	VK_CHECK(vkBindBufferMemory(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), As<VkDeviceMemory>(mMemory), 0));

	if (descriptor.memoryType != MemoryType::GPU)
	{
		VK_CHECK(vkMapMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), 0, descriptor.size, 0, &mContents));
	}
}

Buffer::~Buffer()
{
	vkDestroyBuffer(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), nullptr);
	vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), nullptr);
}

#endif
