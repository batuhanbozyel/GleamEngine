#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Buffer.h"
#include "VulkanUtils.h"

using namespace Gleam;

struct
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
} mContext;

/************************************************************************/
/* Buffer                                                               */
/************************************************************************/
Buffer::Buffer(uint32_t size, BufferUsage usage)
	: mSize(size)
{
	static std::once_flag flag;
	std::call_once(flag, []()
	{
		vkGetPhysicalDeviceMemoryProperties(As<VkPhysicalDevice>(RendererContext::GetPhysicalDevice()), &mContext.memoryProperties);
	});

	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = size;
	createInfo.usage = BufferUsageVkBufferUsage(usage);
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice, &createInfo, nullptr, As<VkBuffer*>(&mBuffer)));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice, As<VkBuffer>(mBuffer), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = [&](uint32_t properties)
	{
		for (uint32_t i = 0; i < mContext.memoryProperties.memoryTypeCount; i++)
		{
			if ((memoryRequirements.memoryTypeBits & BIT(i)) && (mContext.memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		GLEAM_ASSERT(false, "Vulkan: Vertex Buffer suitable memory type could not found!");
		return 0u;
	}(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	VK_CHECK(vkAllocateMemory(VulkanDevice, &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
	VK_CHECK(vkBindBufferMemory(VulkanDevice, As<VkBuffer>(mBuffer), As<VkDeviceMemory>(mMemory), 0));

	vkMapMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), 0, size, 0, &mContents);
}

Buffer::~Buffer()
{
	vkFreeMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), nullptr);
	vkDestroyBuffer(VulkanDevice, As<VkBuffer>(mBuffer), nullptr);
}

void Buffer::SetData(const void* data, uint32_t offset, uint32_t size) const
{
	memcpy(As<uint8_t*>(mContents) + offset, data, size);
}
#endif