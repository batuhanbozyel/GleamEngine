#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Buffer.h"
#include "VulkanDevice.h"

using namespace Gleam;

void Buffer::Allocate()
{
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = mSize;
	createInfo.usage = BufferUsageToVkBufferUsage(mUsage);

	if (mMemoryType == MemoryType::Static)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkBuffer*>(&mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;

	switch (mMemoryType)
	{
		case MemoryType::Static:
		{
			allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			break;
		}
		case MemoryType::Dynamic:
		{
			allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		}
		case MemoryType::Stream:
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

	if (mMemoryType != MemoryType::Static)
	{
		VK_CHECK(vkMapMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), 0, mSize, 0, &mContents));
	}
}

void Buffer::Free()
{
	vkDestroyBuffer(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), nullptr);
	vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), nullptr);
}

#endif
