#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/CommandBuffer.h"

#include "VulkanPipelineStateManager.h"
#include "VulkanShaderReflect.h"
#include "VulkanDevice.h"

#include "Core/Application.h"

using namespace Gleam;

struct CommandBuffer::Impl
{
	VkCommandPool allocatedCommandPool;
	VkCommandBuffer commandBuffer;
	VkFence frameFence;

	const VulkanPipeline* pipeline = nullptr;

	VkRenderPass renderPass;
    bool swapchainTarget = false;

	TArray<TextureDescriptor> colorAttachments;
	TextureDescriptor depthAttachment;
	bool hasDepthAttachment = false;
	uint32_t sampleCount = 1;

	struct TempPool
	{
		TArray<VkRenderPass> renderPasses;
		TArray<VkFramebuffer> framebuffers;

		void Flush()
		{
			for (auto renderPass : renderPasses)
			{
				vkDestroyRenderPass(VulkanDevice::GetHandle(), renderPass, nullptr);
			}
			renderPasses.clear();

			for (auto framebuffer : framebuffers)
			{
				vkDestroyFramebuffer(VulkanDevice::GetHandle(), framebuffer, nullptr);
			}
			framebuffers.clear();
		}
	} tempPool;
};

CommandBuffer::CommandBuffer()
	: mHandle(CreateScope<Impl>())
{
	mHandle->allocatedCommandPool = VulkanDevice::GetSwapchain().GetCommandPool();
	mHandle->frameFence = VulkanDevice::GetSwapchain().GetFence();

	VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	allocateInfo.commandBufferCount = 1;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = mHandle->allocatedCommandPool;
	VK_CHECK(vkAllocateCommandBuffers(VulkanDevice::GetHandle(), &allocateInfo, &mHandle->commandBuffer));
}

CommandBuffer::~CommandBuffer()
{
	VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &mHandle->frameFence, VK_TRUE, UINT64_MAX));
	vkFreeCommandBuffers(VulkanDevice::GetHandle(), mHandle->allocatedCommandPool, 1, &mHandle->commandBuffer);
	mHandle->tempPool.Flush();
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const TStringView debugName) const
{
    mHandle->sampleCount = renderPassDesc.samples;
	mHandle->hasDepthAttachment = renderPassDesc.depthAttachment.texture.IsValid();
    
    uint32_t attachmentCount = static_cast<uint32_t>(renderPassDesc.colorAttachments.size()) + static_cast<uint32_t>(mHandle->hasDepthAttachment);
	TArray<VkAttachmentDescription> attachmentDescriptors(attachmentCount);
	TArray<VkImageView> imageViews(attachmentCount);
	TArray<VkClearValue> clearValues(attachmentCount);
    
    // TODO: Check if we can abstract Vulkan subpasses with Metal tiling API
    // Currently there is no subpass support
    TArray<VkSubpassDescription> subpassDescriptors(1);
    TArray<VkSubpassDependency> subpassDependencies(1);
    if (renderPassDesc.depthAttachment.texture.IsValid())
    {
		mHandle->depthAttachment = renderPassDesc.depthAttachment.texture.GetDescriptor();

        uint32_t depthAttachmentIndex = static_cast<uint32_t>(attachmentDescriptors.size()) - 1;
        
        auto& depthAttachmentDesc = attachmentDescriptors[depthAttachmentIndex];
        depthAttachmentDesc = {};
        depthAttachmentDesc.format = TextureFormatToVkFormat(mHandle->depthAttachment.format);
        depthAttachmentDesc.samples = GetVkSampleCount(renderPassDesc.samples);
        depthAttachmentDesc.loadOp = AttachmentLoadActionToVkAttachmentLoadOp(renderPassDesc.depthAttachment.loadAction);
        depthAttachmentDesc.stencilLoadOp = depthAttachmentDesc.loadOp;
        depthAttachmentDesc.storeOp = AttachmentStoreActionToVkAttachmentStoreOp(renderPassDesc.depthAttachment.storeAction);
        depthAttachmentDesc.stencilStoreOp = depthAttachmentDesc.storeOp;
        depthAttachmentDesc.initialLayout = renderPassDesc.depthAttachment.loadAction == AttachmentLoadAction::Load ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        clearValues[depthAttachmentIndex] = {};
        clearValues[depthAttachmentIndex].depthStencil = { renderPassDesc.depthAttachment.clearDepth, renderPassDesc.depthAttachment.clearStencil };
        imageViews[depthAttachmentIndex] = As<VkImageView>(renderPassDesc.depthAttachment.texture.GetView());
        
        VkAttachmentReference depthStencilAttachment{};
        depthStencilAttachment.attachment = depthAttachmentIndex;
        depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        subpassDescriptors[0].pDepthStencilAttachment = &depthStencilAttachment;
    }
    
    TArray<VkAttachmentReference> colorAttachments(renderPassDesc.colorAttachments.size());
    TArray<VkAttachmentReference> resolveAttachments(renderPassDesc.samples > 1 ? colorAttachments.size() : 0);

	mHandle->colorAttachments.resize(renderPassDesc.colorAttachments.size());
    for (uint32_t i = 0; i < renderPassDesc.colorAttachments.size(); i++)
    {
		const auto& colorAttachment = renderPassDesc.colorAttachments[i];
		mHandle->colorAttachments[i] = colorAttachment.texture.GetDescriptor();

        colorAttachments[i].attachment = i;
        colorAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        
        if (renderPassDesc.samples > 1)
        {
            resolveAttachments[i].attachment = i;
            resolveAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        auto& colorAttachmentDesc = attachmentDescriptors[i];
        colorAttachmentDesc = {};
        colorAttachmentDesc.format = TextureFormatToVkFormat(colorAttachment.texture.GetDescriptor().format);
        colorAttachmentDesc.samples = GetVkSampleCount(renderPassDesc.samples);
        colorAttachmentDesc.loadOp = AttachmentLoadActionToVkAttachmentLoadOp(colorAttachment.loadAction);
        colorAttachmentDesc.storeOp = AttachmentStoreActionToVkAttachmentStoreOp(colorAttachment.storeAction);
        colorAttachmentDesc.initialLayout = colorAttachment.loadAction == AttachmentLoadAction::Load ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        clearValues[i] = { colorAttachment.clearColor.r, colorAttachment.clearColor.g, colorAttachment.clearColor.b, colorAttachment.clearColor.a };
        imageViews[i] = As<VkImageView>(colorAttachment.texture.GetView());
    }

    subpassDescriptors[0] = {};
    subpassDescriptors[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptors[0].colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    subpassDescriptors[0].pColorAttachments = colorAttachments.data();
    subpassDescriptors[0].pResolveAttachments = resolveAttachments.data();

    subpassDependencies[0] = {};
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;
    
	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptors.size());
	renderPassCreateInfo.pAttachments = attachmentDescriptors.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpassDescriptors.size());
	renderPassCreateInfo.pSubpasses = subpassDescriptors.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VK_CHECK(vkCreateRenderPass(VulkanDevice::GetHandle(), &renderPassCreateInfo, nullptr, &mHandle->renderPass));
	mHandle->tempPool.renderPasses.push_back(mHandle->renderPass);

	VkFramebuffer framebuffer;
	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = mHandle->renderPass;
	framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
	framebufferCreateInfo.pAttachments = imageViews.data();
	framebufferCreateInfo.width = static_cast<uint32_t>(renderPassDesc.size.width);
	framebufferCreateInfo.height = static_cast<uint32_t>(renderPassDesc.size.height);
	framebufferCreateInfo.layers = 1;
	VK_CHECK(vkCreateFramebuffer(VulkanDevice::GetHandle(), &framebufferCreateInfo, nullptr, &framebuffer));
	mHandle->tempPool.framebuffers.push_back(framebuffer);

	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = mHandle->renderPass;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(renderPassDesc.size.width);
	renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(renderPassDesc.size.height);
	vkCmdBeginRenderPass(mHandle->commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass() const
{
    vkCmdEndRenderPass(mHandle->commandBuffer);
}

void CommandBuffer::BindGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader) const
{
	if (mHandle->hasDepthAttachment)
	{
		mHandle->pipeline = VulkanPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, mHandle->depthAttachment, vertexShader, fragmentShader, mHandle->renderPass, mHandle->sampleCount);
	}
	else
	{
		mHandle->pipeline = VulkanPipelineStateManager::GetGraphicsPipeline(pipelineDesc, mHandle->colorAttachments, vertexShader, fragmentShader, mHandle->renderPass, mHandle->sampleCount);
	}
	
	vkCmdBindPipeline(mHandle->commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mHandle->pipeline->handle);
}

void CommandBuffer::SetViewport(const Size& size) const
{
	VkViewport viewport{};
	viewport.width = size.width;
	viewport.height = size.height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(mHandle->commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = static_cast<uint32_t>(size.width);
	scissor.extent.height = static_cast<uint32_t>(size.height);
	vkCmdSetScissor(mHandle->commandBuffer, 0, 1, &scissor);
}

void CommandBuffer::BindBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t offset, uint32_t index, ShaderStageFlagBits stage) const
{
	auto resource = [=, this]()
	{
		Shader::Reflection* reflection = nullptr;
		if (stage & ShaderStage_Vertex)
		{
			auto pipeline = static_cast<const VulkanGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->vertexShader->GetReflection().get();
		}
		else if (stage & ShaderStage_Fragment)
		{
			auto pipeline = static_cast<const VulkanGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->fragmentShader->GetReflection().get();
		}
		else
		{
			GLEAM_ASSERT(false, "Metal: Shader stage not implemented yet.")
		}

		switch (usage)
		{
			case BufferUsage::UniformBuffer: return reflection->resources[index];
			case BufferUsage::VertexBuffer:
			case BufferUsage::StorageBuffer:
			{
				return reflection->resources[index + Shader::Reflection::SRV_BINDING_OFFSET]; // TODO: Add support for UAVs
			}
			default: GLEAM_ASSERT(false, "Vulkan: Trying to bind buffer with invalid usage.")
			{
				SpvReflectDescriptorBinding* resource = nullptr;
				return resource;
			}
		}
	}();

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer);
	bufferInfo.offset = offset;
	bufferInfo.range = VK_WHOLE_SIZE;

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = resource->binding;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(usage);
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(mHandle->commandBuffer, mHandle->pipeline->bindPoint, mHandle->pipeline->layout, resource->set, 1, &descriptorSet);
}

void CommandBuffer::BindTexture(const NativeGraphicsHandle texture, uint32_t index, ShaderStageFlagBits stage) const
{
	auto resource = [=, this]()
	{
		Shader::Reflection* reflection = nullptr;
		if (stage & ShaderStage_Vertex)
		{
			auto pipeline = static_cast<const VulkanGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->vertexShader->GetReflection().get();
		}
		else if (stage & ShaderStage_Fragment)
		{
			auto pipeline = static_cast<const VulkanGraphicsPipeline*>(mHandle->pipeline);
			reflection = pipeline->fragmentShader->GetReflection().get();
		}
		else
		{
			GLEAM_ASSERT(false, "Vulkan: Shader stage not implemented yet.")
		}
		return reflection->resources[index + Shader::Reflection::SRV_BINDING_OFFSET]; // TODO: Add support for UAVs
	}();

	VkDescriptorImageInfo imageInfo{};
	imageInfo.imageView = As<VkImageView>(texture);
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // TODO: Set appropriate image layout

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; // TODO: Add support for storage image
	descriptorSet.pImageInfo = &imageInfo;
	vkCmdPushDescriptorSetKHR(mHandle->commandBuffer, mHandle->pipeline->bindPoint, mHandle->pipeline->layout, resource->set, 1, &descriptorSet);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStageFlagBits stage) const
{
    VkShaderStageFlags flagBits{};
    
    if (stage & ShaderStage_Vertex)
        flagBits |= VK_SHADER_STAGE_VERTEX_BIT;
    
    if (stage & ShaderStage_Fragment)
        flagBits |= VK_SHADER_STAGE_FRAGMENT_BIT;
    
    if (stage & ShaderStage_Compute)
        flagBits |= VK_SHADER_STAGE_COMPUTE_BIT;
    
    vkCmdPushConstants(mHandle->commandBuffer, mHandle->pipeline->layout, flagBits, 0, size, data);
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdDraw(mHandle->commandBuffer, vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdBindIndexBuffer(mHandle->commandBuffer, As<VkBuffer>(indexBuffer), 0, static_cast<VkIndexType>(type));
	vkCmdDrawIndexed(mHandle->commandBuffer, indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const NativeGraphicsHandle src, const NativeGraphicsHandle dst, size_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
	VkBufferCopy bufferCopy{};
	bufferCopy.srcOffset = srcOffset;
	bufferCopy.dstOffset = dstOffset;
	bufferCopy.size = size;
	vkCmdCopyBuffer(mHandle->commandBuffer, As<VkBuffer>(src), As<VkBuffer>(dst), 1, &bufferCopy);
}

void CommandBuffer::Blit(const RenderTexture& texture, const RenderTexture& target) const
{
    mHandle->swapchainTarget = !target.IsValid();
	VkImage targetTexture = mHandle->swapchainTarget ? VulkanDevice::GetSwapchain().AcquireNextDrawable().image : As<VkImage>(target.GetHandle());

	if (mHandle->swapchainTarget)
	{
		VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.srcAccessMask = VK_FLAGS_NONE;
		imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.image = targetTexture;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = 0;

		vkCmdPipelineBarrier(mHandle->commandBuffer,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			0, nullptr,
			1, &imageBarrier);
	}

	VkImageCopy region{};
	region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = 1;
	region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.dstSubresource.layerCount = 1;
	region.extent.width = static_cast<uint32_t>(texture.GetDescriptor().size.width);
	region.extent.height = static_cast<uint32_t>(texture.GetDescriptor().size.height);
	region.extent.depth = 1;
	vkCmdCopyImage(mHandle->commandBuffer, As<VkImage>(texture.GetHandle()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	if (mHandle->swapchainTarget)
	{
		VkImageMemoryBarrier imageBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		imageBarrier.image = targetTexture;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseMipLevel = 0;

		vkCmdPipelineBarrier(mHandle->commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_FLAGS_NONE,
			0, nullptr,
			0, nullptr,
			1, &imageBarrier);
	}
}

void CommandBuffer::Begin() const
{
	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(mHandle->commandBuffer, &beginInfo));
}

void CommandBuffer::End() const
{
	VK_CHECK(vkEndCommandBuffer(mHandle->commandBuffer));
}

void CommandBuffer::Commit() const
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &mHandle->commandBuffer;
	VK_CHECK(vkQueueSubmit(VulkanDevice::GetGraphicsQueue().handle, 1, &submitInfo, mHandle->frameFence));
}

void CommandBuffer::Present() const
{
	VulkanDevice::GetSwapchain().Present(mHandle->commandBuffer);
}

void CommandBuffer::WaitUntilCompleted() const
{
	VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &mHandle->frameFence, VK_TRUE, UINT64_MAX));
}

NativeGraphicsHandle CommandBuffer::GetHandle() const
{
	return mHandle->commandBuffer;
}

NativeGraphicsHandle CommandBuffer::GetActiveRenderPass() const
{
	return mHandle->renderPass;
}

#endif
