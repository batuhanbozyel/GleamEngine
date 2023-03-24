#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Texture.h"
#include "VulkanDevice.h"

using namespace Gleam;

Texture2D::Texture2D(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
	VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	createInfo.imageType = VK_IMAGE_TYPE_2D;
	createInfo.format = TextureFormatToVkFormat(descriptor.format);
	createInfo.extent.width = static_cast<uint32_t>(descriptor.size.width);
	createInfo.extent.height = static_cast<uint32_t>(descriptor.size.height);
	createInfo.extent.depth = 1;
	createInfo.arrayLayers = 1;
	createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	createInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	createInfo.mipLevels = mMipMapCount;
	VK_CHECK(vkCreateImage(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkImage*>(&mHandle)));

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), As<VkImage>(mHandle), &memoryRequirements);

	VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
	allocateInfo.allocationSize = memoryRequirements.size;
	allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
	VK_CHECK(vkBindImageMemory(VulkanDevice::GetHandle(), As<VkImage>(mHandle), As<VkDeviceMemory>(mMemory), 0));

	VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	viewCreateInfo.image = As<VkImage>(mHandle);
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = TextureFormatToVkFormat(descriptor.format);
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
	VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &viewCreateInfo, nullptr, As<VkImageView*>(&mView)));
}

Texture2D::~Texture2D()
{
	vkDestroyImageView(VulkanDevice::GetHandle(), As<VkImageView>(mView), nullptr);
	vkDestroyImage(VulkanDevice::GetHandle(), As<VkImage>(mHandle), nullptr);
	vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), nullptr);
}

void Texture2D::SetPixels(const TArray<uint8_t>& pixels) const
{
    // TODO: use staging buffer
}

RenderTexture::RenderTexture(const TextureDescriptor& descriptor)
    : Texture(descriptor)
{
    VkImageUsageFlagBits usage = IsDepthStencilFormat(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = TextureFormatToVkFormat(descriptor.format);
    createInfo.extent.width = static_cast<uint32_t>(descriptor.size.width);
    createInfo.extent.height = static_cast<uint32_t>(descriptor.size.height);
    createInfo.extent.depth = 1;
    createInfo.arrayLayers = 1;
    createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    createInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | usage;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.mipLevels = mMipMapCount;
    VK_CHECK(vkCreateImage(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkImage*>(&mHandle)));

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), As<VkImage>(mHandle), &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMemory)));
    VK_CHECK(vkBindImageMemory(VulkanDevice::GetHandle(), As<VkImage>(mHandle), As<VkDeviceMemory>(mMemory), 0));

    VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
    viewCreateInfo.image = As<VkImage>(mHandle);
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = TextureFormatToVkFormat(descriptor.format);
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
    VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &viewCreateInfo, nullptr, As<VkImageView*>(&mView)));
    
    if (descriptor.sampleCount > 1)
    {
        VkImageUsageFlagBits usage = IsDepthStencilFormat(format) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        VkImageCreateInfo createInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        createInfo.imageType = VK_IMAGE_TYPE_2D;
        createInfo.format = TextureFormatToVkFormat(descriptor.format);
        createInfo.extent.width = static_cast<uint32_t>(descriptor.size.width);
        createInfo.extent.height = static_cast<uint32_t>(descriptor.size.height);
        createInfo.extent.depth = 1;
        createInfo.arrayLayers = 1;
        createInfo.samples = GetVkSampleCount(descriptor.sampleCount);
        createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        createInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | descriptor.usage;
        createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.mipLevels = mMipMapCount;
        VK_CHECK(vkCreateImage(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkImage*>(&mMultisampleHandle)));
        
        VkMemoryRequirements memoryRequirements;
        vkGetImageMemoryRequirements(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), &memoryRequirements);
        
        VkMemoryAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
        // fallback to device local memory if lazily allocated is not present
        if (allocateInfo.memoryTypeIndex == ~0u)
        {
            allocateInfo.memoryTypeIndex = VulkanDevice::GetMemoryTypeForProperties(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        }
        
        VK_CHECK(vkAllocateMemory(VulkanDevice::GetHandle(), &allocateInfo, nullptr, As<VkDeviceMemory*>(&mMultisampleMemory)));
        VK_CHECK(vkBindImageMemory(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), As<VkDeviceMemory>(mMultisampleMemory), 0));
        
        VkImageViewCreateInfo viewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        viewCreateInfo.image = As<VkImage>(mMultisampleHandle);
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = TextureFormatToVkFormat(descriptor.format);
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
        VK_CHECK(vkCreateImageView(VulkanDevice::GetHandle(), &viewCreateInfo, nullptr, As<VkImageView*>(&mMultisampleView)));
    }
}

RenderTexture::~RenderTexture()
{
    vkDestroyImageView(VulkanDevice::GetHandle(), As<VkImageView>(mView), nullptr);
    vkDestroyImage(VulkanDevice::GetHandle(), As<VkImage>(mHandle), nullptr);
    vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMemory), nullptr);
    
    if (mDescriptor.sampleCount > 1)
    {
        vkDestroyImageView(VulkanDevice::GetHandle(), As<VkImageView>(mMultisampleView), nullptr);
        vkDestroyImage(VulkanDevice::GetHandle(), As<VkImage>(mMultisampleHandle), nullptr);
        vkFreeMemory(VulkanDevice::GetHandle(), As<VkDeviceMemory>(mMultisampleMemory), nullptr);
    }
}

#endif
