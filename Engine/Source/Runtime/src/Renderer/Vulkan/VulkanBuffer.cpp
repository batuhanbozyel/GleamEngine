#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Buffer.h"
#include "VulkanUtils.h"

using namespace Gleam;

Buffer::Buffer(uint32_t size, BufferUsage usage)
	: mSize(size)
{
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = BufferUsageVkBufferUsage(usage);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice, &createInfo, nullptr, As<VkBuffer*>(&mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice, As<VkBuffer>(mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = RendererContext::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(VulkanDevice, &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
	VK_CHECK(vkBindBufferMemory(VulkanDevice, As<VkBuffer>(mHandle), As<VkDeviceMemory>(mMemory), 0));

	vkMapMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), 0, size, 0, &mContents);
}

Buffer::~Buffer()
{
	vkDestroyBuffer(VulkanDevice, As<VkBuffer>(mHandle), nullptr);
	vkFreeMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), nullptr);
}

void Buffer::SetData(const void* data, uint32_t offset, uint32_t size) const
{
	memcpy(As<uint8_t*>(mContents) + offset, data, size);
}
#endif