#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/CommandBuffer.h"

#include "VulkanPipelineStateManager.h"
#include "VulkanDevice.h"

#include "Core/Application.h"

#define CurrentCommandBuffer (mHandle->commandBuffers[VulkanDevice::GetSwapchain().GetFrameIndex()])

using namespace Gleam;

struct CommandBuffer::Impl
{
	TArray<VkCommandBuffer> commandBuffers;
	TArray<VkFence> fences;

	VulkanRenderPass renderPassState;
	VulkanPipeline pipelineState;
    bool swapchainTarget = false;

	struct DeletionQueue
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
	};
	TArray<DeletionQueue> deletionQueue;

	void SubmitCommands(VkSubmitInfo submitInfo)
	{
		auto frameIdx = VulkanDevice::GetSwapchain().GetFrameIndex();
		VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &fences[frameIdx], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(VulkanDevice::GetHandle(), 1, &fences[frameIdx]));
		VK_CHECK(vkQueueSubmit(VulkanDevice::GetGraphicsQueue().handle, 1, &submitInfo, fences[frameIdx]));

		deletionQueue[frameIdx].Flush();
		swapchainTarget = false;
	}
};

CommandBuffer::CommandBuffer()
	: mHandle(CreateScope<Impl>())
{
	uint32_t frames = VulkanDevice::GetSwapchain().GetMaxFramesInFlight();
	mHandle->commandBuffers.resize(frames, VK_NULL_HANDLE);
	mHandle->fences.resize(frames, VK_NULL_HANDLE);
	for (uint32_t i = 0; i < frames; i++)
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = VulkanDevice::GetGraphicsCommandPool(i);
		VK_CHECK(vkAllocateCommandBuffers(VulkanDevice::GetHandle(), &allocateInfo, &mHandle->commandBuffers[i]));

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK(vkCreateFence(VulkanDevice::GetHandle(), &fenceCreateInfo, nullptr, &mHandle->fences[i]));
	}
	mHandle->deletionQueue.resize(frames);
}

CommandBuffer::~CommandBuffer()
{
	for (uint32_t i = 0; i < VulkanDevice::GetSwapchain().GetMaxFramesInFlight(); i++)
	{
		VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &mHandle->fences[i], VK_TRUE, UINT64_MAX));
		vkFreeCommandBuffers(VulkanDevice::GetHandle(), VulkanDevice::GetGraphicsCommandPool(i), 1, &mHandle->commandBuffers[i]);
		vkDestroyFence(VulkanDevice::GetHandle(), mHandle->fences[i], nullptr);

		for (auto renderPass : mHandle->deletionQueue[i].renderPasses)
		{
			vkDestroyRenderPass(VulkanDevice::GetHandle(), renderPass, nullptr);
		}
		mHandle->deletionQueue[i].renderPasses.clear();

		for (auto framebuffer : mHandle->deletionQueue[i].framebuffers)
		{
			vkDestroyFramebuffer(VulkanDevice::GetHandle(), framebuffer, nullptr);
		}
		mHandle->deletionQueue[i].framebuffers.clear();
	}
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc) const
{
    mHandle->swapchainTarget = (mActiveRenderTarget == nullptr);
    
	TArray<VkAttachmentDescription> attachmentDescriptors(renderPassDesc.attachments.size());
	TArray<VkImageView> imageViews(renderPassDesc.attachments.size());
	TArray<VkClearValue> clearValues(renderPassDesc.attachments.size());
    
    // TODO: Check if we can abstract Vulkan subpasses with Metal tiling API
    // Currently there is no subpass support
    TArray<VkSubpassDescription> subpassDescriptors(1);
    TArray<VkSubpassDependency> subpassDependencies(1);
	if (mHandle->swapchainTarget)
	{
		mHandle->renderPassState.sampleCount = VulkanDevice::GetSwapchain().GetSampleCount();
		
		attachmentDescriptors.resize(1);
		imageViews.resize(1);
		clearValues.resize(1);

		auto clearColor = ApplicationInstance.backgroundColor;
		if (renderPassDesc.attachments.size() > 0)
			clearColor = renderPassDesc.attachments[0].clearColor;

		clearValues[0] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		imageViews[0] = VulkanDevice::GetSwapchain().AcquireNextDrawable().view;

		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = 0;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		attachmentDescriptors[0] = {};
		attachmentDescriptors[0].format = TextureFormatToVkFormat(VulkanDevice::GetSwapchain().GetFormat());
		attachmentDescriptors[0].samples = GetVkSampleCount(VulkanDevice::GetSwapchain().GetSampleCount());
		attachmentDescriptors[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDescriptors[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDescriptors[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescriptors[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescriptors[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescriptors[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		subpassDescriptors[0] = {};
		subpassDescriptors[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptors[0].colorAttachmentCount = 1;
		subpassDescriptors[0].pColorAttachments = &attachmentRef;

		subpassDependencies[0] = {};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;
	}
	else
	{
		mHandle->renderPassState.sampleCount = renderPassDesc.samples;
		
        if (mActiveRenderTarget->HasDepthBuffer())
		{
            VkAttachmentReference depthStencilAttachment{};
			depthStencilAttachment.attachment = renderPassDesc.depthAttachmentIndex;
			depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			const auto& depthBuffer = mActiveRenderTarget->GetDepthBuffer();
			const auto& depthAttachment = renderPassDesc.attachments[renderPassDesc.depthAttachmentIndex];

			auto& depthAttachmentDesc = attachmentDescriptors[renderPassDesc.depthAttachmentIndex];
			depthAttachmentDesc.format = TextureFormatToVkFormat(depthAttachment.format);
			depthAttachmentDesc.samples = GetVkSampleCount(renderPassDesc.samples);
			depthAttachmentDesc.loadOp = AttachmentLoadActionToVkAttachmentLoadOp(depthAttachment.loadAction);
			depthAttachmentDesc.stencilLoadOp = depthAttachmentDesc.loadOp;
			depthAttachmentDesc.storeOp = AttachmentStoreActionToVkAttachmentStoreOp(depthAttachment.storeAction);
			depthAttachmentDesc.stencilStoreOp = depthAttachmentDesc.storeOp;
			depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			VkClearValue clearValue{};
			clearValue.depthStencil = { depthAttachment.clearDepth, depthAttachment.clearStencil };
			clearValues[renderPassDesc.depthAttachmentIndex] = clearValue;

			imageViews[renderPassDesc.depthAttachmentIndex] = As<VkImageView>(depthBuffer->GetImageView());
			subpassDescriptors[0].pDepthStencilAttachment = &depthStencilAttachment;
		}
        
        TArray<VkAttachmentReference> colorAttachments(mActiveRenderTarget->GetColorBuffers().size());
		for (uint32_t i = 0; i < mActiveRenderTarget->GetColorBuffers().size(); i++)
		{
			uint32_t attachmentIndexOffset = static_cast<uint32_t>(mActiveRenderTarget->HasDepthBuffer() && (renderPassDesc.depthAttachmentIndex <= static_cast<int>(i)));
			uint32_t colorAttachmentIndex = i + attachmentIndexOffset;

			colorAttachments[i].attachment = colorAttachmentIndex;
			colorAttachments[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			const auto& colorBuffer = mActiveRenderTarget->GetColorBuffers()[i];
			const auto& colorAttachment = renderPassDesc.attachments[colorAttachmentIndex];

			auto& colorAttachmentDesc = attachmentDescriptors[colorAttachmentIndex];
			colorAttachmentDesc.format = TextureFormatToVkFormat(colorAttachment.format);
			colorAttachmentDesc.samples = GetVkSampleCount(renderPassDesc.samples);
			colorAttachmentDesc.loadOp = AttachmentLoadActionToVkAttachmentLoadOp(colorAttachment.loadAction);
			colorAttachmentDesc.storeOp = AttachmentStoreActionToVkAttachmentStoreOp(colorAttachment.storeAction);
			colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

			clearValues[colorAttachmentIndex] = { colorAttachment.clearColor.r, colorAttachment.clearColor.g, colorAttachment.clearColor.b, colorAttachment.clearColor.a };
			imageViews[colorAttachmentIndex] = As<VkImageView>(colorBuffer->GetImageView());
		}

		subpassDescriptors[0] = {};
		subpassDescriptors[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDescriptors[0].colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		subpassDescriptors[0].pColorAttachments = colorAttachments.data();

		subpassDependencies[0] = {};
		subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependencies[0].dstSubpass = 0;
		subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependencies[0].dependencyFlags = 0;
	}

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptors.size());
	renderPassCreateInfo.pAttachments = attachmentDescriptors.data();
	renderPassCreateInfo.subpassCount = static_cast<uint32_t>(subpassDescriptors.size());
	renderPassCreateInfo.pSubpasses = subpassDescriptors.data();
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VK_CHECK(vkCreateRenderPass(VulkanDevice::GetHandle(), &renderPassCreateInfo, nullptr, &mHandle->renderPassState.handle));
	mHandle->deletionQueue[VulkanDevice::GetSwapchain().GetFrameIndex()].renderPasses.push_back(mHandle->renderPassState.handle);

	VkFramebuffer framebuffer;
	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = mHandle->renderPassState.handle;
	framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
	framebufferCreateInfo.pAttachments = imageViews.data();
	framebufferCreateInfo.width = static_cast<uint32_t>(renderPassDesc.size.width);
	framebufferCreateInfo.height = static_cast<uint32_t>(renderPassDesc.size.height);
	framebufferCreateInfo.layers = 1;
	VK_CHECK(vkCreateFramebuffer(VulkanDevice::GetHandle(), &framebufferCreateInfo, nullptr, &framebuffer));
	mHandle->deletionQueue[VulkanDevice::GetSwapchain().GetFrameIndex()].framebuffers.push_back(framebuffer);

	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = mHandle->renderPassState.handle;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.renderArea.extent.width = static_cast<uint32_t>(renderPassDesc.size.width);
	renderPassBeginInfo.renderArea.extent.height = static_cast<uint32_t>(renderPassDesc.size.height);
	vkCmdBeginRenderPass(CurrentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass() const
{
    vkCmdEndRenderPass(CurrentCommandBuffer);
    mHandle->renderPassState.handle = VK_NULL_HANDLE;
    mHandle->renderPassState.sampleCount = 1;
}

void CommandBuffer::BindPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<RefCounted<Shader>>& program) const
{
	mHandle->pipelineState = VulkanPipelineStateManager::GetPipeline(pipelineDesc, program, mHandle->renderPassState);
	vkCmdBindPipeline(CurrentCommandBuffer, mHandle->pipelineState.bindPoint, mHandle->pipelineState.handle);
}

void CommandBuffer::SetViewport(const Size& size) const
{
	VkViewport viewport{};
	viewport.width = size.width;
	viewport.height = size.height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(CurrentCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = static_cast<uint32_t>(size.width);
	scissor.extent.height = static_cast<uint32_t>(size.height);
	vkCmdSetScissor(CurrentCommandBuffer, 0, 1, &scissor);
}

void CommandBuffer::SetVertexBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, uint32_t index, uint32_t offset) const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer);
	bufferInfo.offset = offset;
	bufferInfo.range = size;

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(usage);
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, mHandle->pipelineState.bindPoint, mHandle->pipelineState.layout, 0, 1, &descriptorSet);
}

void CommandBuffer::SetFragmentBuffer(const NativeGraphicsHandle buffer, BufferUsage usage, size_t size, uint32_t index, uint32_t offset) const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer);
	bufferInfo.range = size;

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(usage);
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, mHandle->pipelineState.bindPoint, mHandle->pipelineState.layout, 1, 1, &descriptorSet);
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
    
    vkCmdPushConstants(CurrentCommandBuffer, mHandle->pipelineState.layout, flagBits, 0, size, data);
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdDraw(CurrentCommandBuffer, vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const NativeGraphicsHandle indexBuffer, IndexType type, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdBindIndexBuffer(CurrentCommandBuffer, As<VkBuffer>(indexBuffer), 0, static_cast<VkIndexType>(type));
	vkCmdDrawIndexed(CurrentCommandBuffer, indexCount, instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const IBuffer& src, const IBuffer& dst, size_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
	VkBufferCopy bufferCopy{};
	bufferCopy.srcOffset = srcOffset;
	bufferCopy.dstOffset = dstOffset;
	bufferCopy.size = size;
	vkCmdCopyBuffer(CurrentCommandBuffer, As<VkBuffer>(src.GetHandle()), As<VkBuffer>(dst.GetHandle()), 1, &bufferCopy);
}

void CommandBuffer::Blit(const RenderTexture& texture, const Optional<RenderTexture>& target) const
{
    mHandle->swapchainTarget = !target.has_value();
	VkImage targetTexture = mHandle->swapchainTarget ? VulkanDevice::GetSwapchain().AcquireNextDrawable().image : As<VkImage>(target.value().GetHandle());

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

		vkCmdPipelineBarrier(CurrentCommandBuffer,
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
	region.extent.width = static_cast<uint32_t>(texture.GetSize().width);
	region.extent.height = static_cast<uint32_t>(texture.GetSize().height);
	region.extent.depth = 1;
	vkCmdCopyImage(CurrentCommandBuffer, As<VkImage>(texture.GetHandle()), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, targetTexture, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

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

		vkCmdPipelineBarrier(CurrentCommandBuffer,
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
	VK_CHECK(vkBeginCommandBuffer(CurrentCommandBuffer, &beginInfo));
}

void CommandBuffer::End() const
{
	VK_CHECK(vkEndCommandBuffer(CurrentCommandBuffer));
}

void CommandBuffer::Commit() const
{
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CurrentCommandBuffer;
	mHandle->SubmitCommands(submitInfo);
}

void CommandBuffer::Present() const
{
	VkSemaphore waitSemaphore = VulkanDevice::GetSwapchain().GetImageAcquireSemaphore();
	VkSemaphore signalSemaphore = VulkanDevice::GetSwapchain().GetImageReleaseSemaphore();

	VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &waitSemaphore;
	submitInfo.pWaitDstStageMask = &waitDstStageMask;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &CurrentCommandBuffer;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &signalSemaphore;

	mHandle->SubmitCommands(submitInfo);
	VulkanDevice::GetSwapchain().Present();

	uint32_t frameIdx = VulkanDevice::GetSwapchain().GetFrameIndex();
	VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &mHandle->fences[frameIdx], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetCommandPool(VulkanDevice::GetHandle(), VulkanDevice::GetGraphicsCommandPool(frameIdx), 0));
}

void CommandBuffer::WaitUntilCompleted() const
{
	for (auto fence : mHandle->fences)
		VK_CHECK(vkWaitForFences(VulkanDevice::GetHandle(), 1, &fence, VK_TRUE, UINT64_MAX));
}

#endif
