#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Texture.h"
#include "VulkanDevice.h"

using namespace Gleam;

Texture::Texture()
	: mDescriptor({
		.size = VulkanDevice::GetSwapchain().GetSize(),
		.format = VulkanDevice::GetSwapchain().GetFormat(),
		.type = TextureType::RenderTexture
	}), mMipMapLevels(1)
{

}

Texture::Texture(const TextureDescriptor& descriptor)
	: mDescriptor(descriptor), mMipMapLevels(descriptor.useMipMap ? CalculateMipLevels(descriptor.size) : 1)
{
	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	if (mDescriptor.type == TextureType::TextureCube)
	{
		float size = Math::Min(mDescriptor.size.width, mDescriptor.size.height);
		mDescriptor.size.width = size;
		mDescriptor.size.height = size;

		createInfo.imageType = VK_IMAGE_TYPE_3D;
		createInfo.usage = usage;
	}
	else
	{
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.usage = mDescriptor.type == TextureType::RenderTexture ? usage | (Utils::IsColorFormat(descriptor.format) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) : usage;
	}
	createInfo.format = TextureFormatToVkFormat(descriptor.format);
	createInfo.extent.width = static_cast<uint32_t>(descriptor.size.width);
	createInfo.extent.height = static_cast<uint32_t>(descriptor.size.height);
	createInfo.extent.depth = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.mipLevels = mMipMapLevels;
	VK_CHECK(vkCreateImage(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkImage*>(&mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), As<VkImage>(mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mHeap)));
	VK_CHECK(vkBindImageMemory(VulkanDevice::GetHandle(), As<VkImage>(mHandle), As<VkDeviceMemory>(mHeap), 0));

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.image = As<VkImage>(mHandle);
	viewCreateInfo.viewType = mDescriptor.type == TextureType::TextureCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = createInfo.format;
	viewCreateInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY
	};
	viewCreateInfo.subresourceRange.layerCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;

	if (Utils::IsColorFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (Utils::IsDepthFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	if (Utils::IsStencilFormat(descriptor.format))
	{
		viewCreateInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &viewCreateInfo, nullptr, As<VkImageView*>(&mView)));

	if (mDescriptor.sampleCount > 1)
	{
		createInfo.samples = GetVkSampleCount(descriptor.sampleCount);
		createInfo.usage = Utils::IsColorFormat(descriptor.format) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // TODO: Switch to memoryless msaa render targets when Tile shading is supported
		VK_CHECK(vkCreateImage(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkImage*>(&mMultisampleHandle)));

		VkMemoryRequirements memoryRequirements;
		vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), &memoryRequirements);

		VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocateInfo.allocationSize = memoryRequirements.size;
		allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // TODO: Switch to memoryless msaa render targets when Tile shading is supported
		VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMultisampleHeap)));
		VK_CHECK(vkBindImageMemory(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), As<VkDeviceMemory>(mMultisampleHeap), 0));

		viewCreateInfo.image = As<VkImage>(mMultisampleHandle);
		VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &viewCreateInfo, nullptr, As<VkImageView*>(&mMultisampleView)));
	}
}

void Texture::SetPixels(const void* pixels) const
{
	
}

void Texture::Dispose()
{
    if (mHandle == nullptr) return;
    
    vkDestroyImageView(VulkanDevice::GetHandle(), As<VkImageView>(mView), nullptr);
    vkDestroyImage(VulkanDevice::GetHandle(), As<VkImage>(mHandle), nullptr);
    vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mHeap), nullptr);
    
    if (mDescriptor.sampleCount > 1)
    {
        vkDestroyImageView(VulkanDevice::GetHandle(), As<VkImageView>(mMultisampleView), nullptr);
        vkDestroyImage(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), nullptr);
        vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMultisampleHeap), nullptr);
    }
    
    mHeap = nullptr;
    mView = nullptr;
    mHandle = nullptr;
    mMultisampleHeap = nullptr;
    mMultisampleView = nullptr;
    mMultisampleHandle = nullptr;
}

#endif
