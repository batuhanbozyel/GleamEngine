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

	VmaAllocationCreateInfo vmaCreateInfo{};
	vmaCreateInfo.usage = MemoryTypeToVmaMemoryUsage(descriptor.memoryType);
	vmaCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
	vmaCreateInfo.priority = 1.0f;
	VmaAllocationInfo vmaAllocationInfo{};
	VK_CHECK(vmaAllocateMemoryForBuffer(VulkanDevice::GetAllocator(), buffer, &vmaCreateInfo, As<VmaAllocation*>(&mHandle), &vmaAllocationInfo));
	vkDestroyBuffer(VulkanDevice::GetHandle(), buffer, nullptr);
}

void Heap::Dispose()
{
	if (mDescriptor.memoryType != MemoryType::GPU)
	{
		vmaUnmapMemory(VulkanDevice::GetAllocator(), As<VmaAllocation>(mHandle));
	}
	vmaFreeMemory(VulkanDevice::GetAllocator(), As<VmaAllocation>(mHandle));
}

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor, size_t offset) const
{
	VkBuffer buffer;
    VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | BufferUsageToVkBufferUsage(descriptor.usage);

	if (mDescriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice::GetHandle(), &createInfo, nullptr, &buffer));
	VK_CHECK(vmaBindBufferMemory2(VulkanDevice::GetAllocator(), As<VmaAllocation>(mHandle), offset, buffer, nullptr));

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		VK_CHECK(vmaMapMemory(VulkanDevice::GetAllocator(), As<VmaAllocation>(mHandle), &contents));
		contents = static_cast<uint8_t*>(contents) + offset;
	}
    return Buffer(buffer, descriptor, contents);
}

void Buffer::Dispose()
{
    vkDestroyBuffer(VulkanDevice::GetHandle(), As<VkBuffer>(mHandle), nullptr);
}

#endif
