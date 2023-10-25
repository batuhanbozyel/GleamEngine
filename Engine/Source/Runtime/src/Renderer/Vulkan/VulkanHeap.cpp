#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Heap.h"
#include "Renderer/Buffer.h"

#include "VulkanDevice.h"
#include "VulkanUtils.h"

using namespace Gleam;

Buffer Heap::CreateBuffer(const BufferDescriptor& descriptor)
{
	auto alignedStackPtr = Utils::AlignTo(mStackPtr, mAlignment);
	auto newStackPtr = alignedStackPtr + descriptor.size;

	if (Utils::AlignTo(mDescriptor.size, mAlignment) < newStackPtr)
	{
		GLEAM_ASSERT(false, "Vulkan: Heap is full!");
		return Buffer(nullptr, descriptor, nullptr);
	}
	mStackPtr = newStackPtr;

	VkBuffer buffer = VK_NULL_HANDLE;
    VkBufferCreateInfo createInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	createInfo.size = descriptor.size;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | BufferUsageToVkBufferUsage(descriptor.usage);

	if (mDescriptor.memoryType == MemoryType::GPU)
	{
		createInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

	VK_CHECK(vkCreateBuffer(As<VkDevice>(mDevice->GetHandle()), &createInfo, nullptr, &buffer));
	VK_CHECK(vmaBindBufferMemory2(As<const VulkanDevice*>(mDevice)->GetAllocator(), As<VmaAllocation>(mHandle), alignedStackPtr, buffer, nullptr));

    void* contents = nullptr;
    if (mDescriptor.memoryType != MemoryType::GPU)
	{
		VK_CHECK(vmaMapMemory(As<const VulkanDevice*>(mDevice)->GetAllocator(), As<VmaAllocation>(mHandle), &contents));
		contents = static_cast<uint8_t*>(contents) + alignedStackPtr;
	}
    return Buffer(buffer, descriptor, contents);
}

#endif
