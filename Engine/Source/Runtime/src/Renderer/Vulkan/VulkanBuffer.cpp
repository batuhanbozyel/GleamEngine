#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Buffer.h"
#include "VulkanUtils.h"

using namespace Gleam;

Buffer::Buffer(uint32_t size, BufferUsage usage, MemoryType memoryType)
	: mSize(size), mUsage(usage), mMemoryType(memoryType)
{
	Allocate(*this);
}

Buffer::Buffer(const void* data, uint32_t size, BufferUsage usage, MemoryType memoryType)
	: Buffer(size, usage, memoryType)
{
	SetData(data, 0, size);
}

Buffer::~Buffer()
{
	Free(*this);
}

void Buffer::Allocate(Buffer& buffer)
{
	VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = buffer.GetSize();
	createInfo.usage = BufferUsageToVkBufferUsage(buffer.GetUsage());

	if (buffer.mMemoryType == MemoryType::Static)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}

	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	VK_CHECK(vkCreateBuffer(VulkanDevice, &createInfo, nullptr, As<VkBuffer*>(&buffer.mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(VulkanDevice, As<VkBuffer>(buffer.mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;

	switch (buffer.mMemoryType)
	{
		case MemoryType::Static:
		{
			allocateInfo.memoryTypeIndex = RendererContext::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			break;
		}
		case MemoryType::Dynamic:
		{
			allocateInfo.memoryTypeIndex = RendererContext::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		}
		case MemoryType::Stream:
		{
			allocateInfo.memoryTypeIndex = RendererContext::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			break;
		}
		default:
		{
			GLEAM_ASSERT(false, "Vulkan: Unknown memory type given!");
			break;
		}
	}

	VK_CHECK(vkAllocateMemory(VulkanDevice, &allocateInfo, nullptr, As<VkDeviceMemory*>(&buffer.mMemory)));
	VK_CHECK(vkBindBufferMemory(VulkanDevice, As<VkBuffer>(buffer.mHandle), As<VkDeviceMemory>(buffer.mMemory), 0));

	if (buffer.mMemoryType != MemoryType::Static)
	{
		VK_CHECK(vkMapMemory(VulkanDevice, As<VkDeviceMemory>(buffer.mMemory), 0, buffer.mSize, 0, &buffer.mContents));
	}
}

void Buffer::Free(const Buffer& buffer)
{
	vkDestroyBuffer(VulkanDevice, As<VkBuffer>(buffer.mHandle), nullptr);
	vkFreeMemory(VulkanDevice, As<VkDeviceMemory>(buffer.mMemory), nullptr);
}

void Buffer::Copy(const Buffer& src, const Buffer& dst)
{
	GLEAM_ASSERT(src.GetSize() <= dst.GetSize(), "Vulkan: Source buffer size can not be larger than destination buffer size!");

	VkBufferCopy bufferCopy;
	bufferCopy.srcOffset = 0;
	bufferCopy.dstOffset = 0;
	bufferCopy.size = src.GetSize();

	auto frameIdx = RendererContext::GetSwapchain()->GetFrameIndex();
	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = As<VkCommandPool>(RendererContext::GetTransferCommandPool(frameIdx));

	VkCommandBuffer commandBuffer;
	VK_CHECK(vkAllocateCommandBuffers(VulkanDevice, &allocateInfo, &commandBuffer));

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

	vkCmdCopyBuffer(commandBuffer, As<VkBuffer>(src.GetHandle()), As<VkBuffer>(dst.GetHandle()), 1, &bufferCopy);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK(vkQueueSubmit(As<VkQueue>(RendererContext::GetTransferQueue()), 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK(vkQueueWaitIdle(As<VkQueue>(RendererContext::GetTransferQueue())));

	vkFreeCommandBuffers(VulkanDevice, As<VkCommandPool>(RendererContext::GetTransferCommandPool(frameIdx)), 1, &commandBuffer);
}

void Buffer::Resize(uint32_t size, bool keepContent)
{
	if (keepContent)
	{
		if (mMemoryType == MemoryType::Static)
		{
			Buffer stagingBuffer(mSize, BufferUsage::StagingBuffer, MemoryType::Static);
			Copy(*this, stagingBuffer);
			Free(*this);

			mSize = size;
			Allocate(*this);

			Copy(stagingBuffer, *this);
		}
		else
		{
			TArray<uint8_t> stagingBuffer(mSize);
			memcpy(stagingBuffer.data(), mContents, mSize);
			Free(*this);

			mSize = size;
			Allocate(*this);

			memcpy(mContents, stagingBuffer.data(), stagingBuffer.size());
		}
	}
	else
	{
		Free(*this);
		Allocate(*this);
	}
}

void Buffer::SetData(const void* data, uint32_t offset, uint32_t size) const
{
	if (mMemoryType == MemoryType::Static)
	{
		Buffer stagingBuffer(data, mSize, BufferUsage::StagingBuffer, MemoryType::Stream);
		Copy(stagingBuffer, *this);
	}
	else
	{
		memcpy(As<uint8_t*>(mContents) + offset, data, size);
	}
}
#endif