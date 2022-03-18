#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/RenderPass.h"
#include "VulkanUtils.h"

#include "Renderer/Texture.h"

using namespace Gleam;

RenderPass::RenderPass(const TArray<Subpass>& subpasses, const TArray<RenderTexture>& attachments, int depthAttachmentIndex)
{
	TArray<VkAttachmentDescription> attachmentDescriptions(attachments.size());
	for (uint32_t i = 0; i < attachments.size(); i++)
	{
		VkAttachmentDescriptionFlags    flags;
		VkFormat                        format;
		VkSampleCountFlagBits           samples;
		VkAttachmentLoadOp              loadOp;
		VkAttachmentStoreOp             storeOp;
		VkAttachmentLoadOp              stencilLoadOp;
		VkAttachmentStoreOp             stencilStoreOp;
		VkImageLayout                   initialLayout;
		VkImageLayout                   finalLayout;
	}

	TArray<VkSubpassDescription> subpassDescriptions(subpasses.size());
	for (uint32_t i = 0; i < subpasses.size(); i++)
	{
		VkSubpassDescriptionFlags       flags;
		VkPipelineBindPoint             pipelineBindPoint;
		uint32_t                        inputAttachmentCount;
		const VkAttachmentReference* pInputAttachments;
		uint32_t                        colorAttachmentCount;
		const VkAttachmentReference* pColorAttachments;
		const VkAttachmentReference* pResolveAttachments;
		const VkAttachmentReference* pDepthStencilAttachment;
		uint32_t                        preserveAttachmentCount;
		const uint32_t* pPreserveAttachments;
	}


	VkRenderPassCreateInfo createInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	createInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	createInfo.pAttachments = attachmentDescriptions.data();
	createInfo.subpassCount = static_cast<uint32_t>(subpasses.size());
	createInfo.pSubpasses = subpassDescriptions.data();
	uint32_t                          dependencyCount;
	const VkSubpassDependency* pDependencies;

	VK_CHECK(vkCreateRenderPass(VulkanDevice, &createInfo, nullptr, As<VkRenderPass*>(&mHandle)));
}

RenderPass::~RenderPass()
{
	vkDestroyRenderPass(VulkanDevice, As<VkRenderPass>(mHandle), nullptr);
}

#endif