#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Texture.h"
#include "VulkanUtils.h"

using namespace Gleam;

Texture2D::Texture2D(uint32_t width, uint32_t height, TextureFormat format, bool useMipMap)
	: mWidth(width), mHeight(height), mFormat(format), mMipMapCount(useMipMap ? CalculateMipLevels(width, height) : 1)
{
	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = TextureFormatToVkFormat(mFormat);
	createInfo.extent.width = mWidth;
	createInfo.extent.height = mHeight;
	createInfo.extent.depth = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.mipLevels = mMipMapCount;
	VK_CHECK(vkCreateImage(VulkanDevice, &createInfo, nullptr, As<VkImage*>(&mImage)));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(VulkanDevice, As<VkImage>(mImage), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = RendererContext::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	VK_CHECK(vkAllocateMemory(VulkanDevice, &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
	VK_CHECK(vkBindImageMemory(VulkanDevice, As<VkImage>(mImage), As<VkDeviceMemory>(mMemory), 0));

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.image = As<VkImage>(mImage);
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = TextureFormatToVkFormat(mFormat);
	viewCreateInfo.components = {
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY,
			VK_COMPONENT_SWIZZLE_IDENTITY
	};
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	VK_CHECK(vkCreateImageView(VulkanDevice, &viewCreateInfo, nullptr, As<VkImageView*>(mImageView)));
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(VulkanDevice, As<VkImageView>(mImageView), nullptr);
	vkDestroyImage(VulkanDevice, As<VkImage>(mImage), nullptr);
	vkFreeMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), nullptr);
}

void Texture2D::SetPixels(const TArray<uint8_t>& pixels) const
{
	void* memoryPtr;
	VK_CHECK(vkMapMemory(VulkanDevice, As<VkDeviceMemory>(mMemory), 0, pixels.size(), 0, &memoryPtr));
	memcpy(As<uint8_t*>(memoryPtr), pixels.data(), pixels.size());
	vkUnmapMemory(VulkanDevice, As<VkDeviceMemory>(mMemory));

	// TODO: generate mipmaps
}

#endif