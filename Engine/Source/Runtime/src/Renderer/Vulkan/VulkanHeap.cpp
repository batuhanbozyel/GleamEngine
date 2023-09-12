#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"
#include "VulkanDevice.h"

using namespace Gleam;

Heap::Heap(const HeapDescriptor& descriptor)
    : mDescriptor(descriptor)
{
	VkBuffer buffer;
    VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	if (mDescriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, &buffer));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice::GetHandle(), buffer, &memoryRequirements);

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

	VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mHandle)));
	vkDestroyBuffer(VulkanDevice::GetHandle(), buffer, nullptr);
}

Heap::~Heap()
{
	vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mHandle), nullptr);
}

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
	VkBuffer buffer;
    VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = BufferUsageToVkBufferUsage(descriptor.usage);

	if (mDescriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, &buffer));
	VK_CHECK(vkBindBufferMemory(VulkanDevice::GetHandle(), buffer, As<VkDeviceMemory>(mHandle), offset));

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		VK_CHECK(vkMapMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mHandle), offset, descriptor.size, 0, &contents));
	}
    return Buffer(buffer, descriptor, contents);
}

void Heap::DestroyBuffer(const Buffer& buffer) const
{
    vkDestroyBuffer(VulkanDevice::GetHandle(), As<VkBuffer>(buffer.GetHandle()), nullptr);
}

#endif
