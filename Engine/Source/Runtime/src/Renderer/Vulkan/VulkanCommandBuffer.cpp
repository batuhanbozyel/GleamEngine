#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/CommandBuffer.h"
#include "VulkanPipelineStateManager.h"

#include "Renderer/Renderer.h"
#include "Renderer/ShaderLibrary.h"

#define CurrentCommandBuffer (mHandle->commandBuffers[RendererContext::GetSwapchain()->GetFrameIndex()])

using namespace Gleam;

struct CommandBuffer::Impl
{
	TArray<VkCommandBuffer> commandBuffers;
	TArray<VkFence> fences;

	VulkanRenderPass renderPassState;
	VulkanPipeline pipelineState;

	struct DeletionQueue
	{
		TArray<VkFramebuffer> framebuffers;
	};
	TArray<DeletionQueue> deletionQueue;
};

CommandBuffer::CommandBuffer()
	: mHandle(CreateScope<Impl>())
{
	mHandle->commandBuffers.resize(RendererContext::GetProperties().maxFramesInFlight, VK_NULL_HANDLE);
	mHandle->fences.resize(RendererContext::GetProperties().maxFramesInFlight, VK_NULL_HANDLE);
	for (uint32_t i = 0; i < RendererContext::GetProperties().maxFramesInFlight; i++)
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(i));
		VK_CHECK(vkAllocateCommandBuffers(VulkanDevice, &allocateInfo, &mHandle->commandBuffers[i]));

		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK(vkCreateFence(VulkanDevice, &fenceCreateInfo, nullptr, &mHandle->fences[i]));
	}
	mHandle->deletionQueue.resize(RendererContext::GetProperties().maxFramesInFlight);
}

CommandBuffer::~CommandBuffer()
{
	for (uint32_t i = 0; i < RendererContext::GetProperties().maxFramesInFlight; i++)
	{
		VK_CHECK(vkWaitForFences(VulkanDevice, 1, &mHandle->fences[i], VK_TRUE, UINT64_MAX));
		vkFreeCommandBuffers(VulkanDevice, As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(i)), 1,&mHandle->commandBuffers[i]);
		vkDestroyFence(VulkanDevice, mHandle->fences[i], nullptr);

		for (auto framebuffer : mHandle->deletionQueue[i].framebuffers)
		{
			vkDestroyFramebuffer(VulkanDevice, framebuffer, nullptr);
		}
		mHandle->deletionQueue[i].framebuffers.clear();
	}
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc) const
{
	mHandle->renderPassState = VulkanPipelineStateManager::GetRenderPass(renderPassDesc);

	TArray<VkImageView> imageViews(renderPassDesc.attachments.size());
	TArray<VkClearValue> clearValues(renderPassDesc.attachments.size());
	if (renderPassDesc.swapchainTarget)
	{
		if (renderPassDesc.attachments.size() > 0)
		{
			const auto& clearColor = renderPassDesc.attachments[0].clearColor;
			clearValues[0] = { clearColor.r, clearColor.g, clearColor.b, clearColor.a };
		}
		else
		{
			const auto& clearColor = Renderer::clearColor;
			clearValues.emplace_back(VkClearValue{ clearColor.r, clearColor.g, clearColor.b, clearColor.a });
		}

		auto frameIdx = RendererContext::GetSwapchain()->GetFrameIndex();
		VK_CHECK(vkWaitForFences(VulkanDevice, 1, &mHandle->fences[frameIdx], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetFences(VulkanDevice, 1, &mHandle->fences[frameIdx]));

		imageViews.push_back(As<VkImageView>(RendererContext::GetSwapchain()->AcquireNextDrawable()));
	}
	else
	{
		// TODO:
	}

	VkFramebuffer framebuffer;
	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = mHandle->renderPassState.handle;
	framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(imageViews.size());
	framebufferCreateInfo.pAttachments = imageViews.data();
	framebufferCreateInfo.width = renderPassDesc.width;
	framebufferCreateInfo.height = renderPassDesc.height;
	framebufferCreateInfo.layers = 1;
	VK_CHECK(vkCreateFramebuffer(VulkanDevice, &framebufferCreateInfo, nullptr, &framebuffer));
	mHandle->deletionQueue[RendererContext::GetSwapchain()->GetFrameIndex()].framebuffers.push_back(framebuffer);

	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = mHandle->renderPassState.handle;
	renderPassBeginInfo.framebuffer = framebuffer;
	renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.renderArea.extent.width = renderPassDesc.width;
	renderPassBeginInfo.renderArea.extent.height = renderPassDesc.height;
	vkCmdBeginRenderPass(CurrentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void CommandBuffer::EndRenderPass() const
{
	vkCmdEndRenderPass(CurrentCommandBuffer);
}

void CommandBuffer::BindPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
	mHandle->pipelineState = VulkanPipelineStateManager::GetGraphicsPipeline(pipelineDesc, program, mHandle->renderPassState);
	vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mHandle->pipelineState.handle);
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height) const
{
	VkViewport viewport{};
	viewport.width = static_cast<float>(width);
	viewport.height = static_cast<float>(height);
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(CurrentCommandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(CurrentCommandBuffer, 0, 1, &scissor);
}

void CommandBuffer::SetVertexBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer.GetHandle());
	bufferInfo.offset = offset;
	bufferInfo.range = buffer.GetSize();

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(buffer.GetUsage());
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mHandle->pipelineState.layout, 0, 1, &descriptorSet);
}

void CommandBuffer::SetFragmentBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer.GetHandle());
	bufferInfo.range = buffer.GetSize();

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(buffer.GetUsage());
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mHandle->pipelineState.layout, 1, 1, &descriptorSet);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStage stage, uint32_t index) const
{
	switch (stage)
	{
		case ShaderStage::Vertex:
		{
			vkCmdPushConstants(CurrentCommandBuffer, mHandle->pipelineState.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
			break;
		}
		case ShaderStage::Fragment:
		{
			vkCmdPushConstants(CurrentCommandBuffer, mHandle->pipelineState.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, data);
			break;
		}
		case ShaderStage::Compute:
		{
			vkCmdPushConstants(CurrentCommandBuffer, mHandle->pipelineState.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);
			break;
		}
	}
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdDraw(CurrentCommandBuffer, vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
	vkCmdBindIndexBuffer(CurrentCommandBuffer, As<VkBuffer>(indexBuffer.GetHandle()), 0, static_cast<VkIndexType>(indexBuffer.GetIndexType()));
	vkCmdDrawIndexed(CurrentCommandBuffer, indexBuffer.GetCount(), instanceCount, firstIndex, baseVertex, baseInstance);
}

void CommandBuffer::CopyBuffer(const Buffer& src, const Buffer& dst, uint32_t size, uint32_t srcOffset, uint32_t dstOffset) const
{
	GLEAM_ASSERT(size <= src.GetSize(), "Vulkan: Copy size can not be larger than source buffer size!");
	GLEAM_ASSERT(size <= dst.GetSize(), "Vulkan: Copy size can not be larger than destination buffer size!");

	VkBufferCopy bufferCopy{};
	bufferCopy.srcOffset = srcOffset;
	bufferCopy.dstOffset = dstOffset;
	bufferCopy.size = size;
	vkCmdCopyBuffer(CurrentCommandBuffer, As<VkBuffer>(src.GetHandle()), As<VkBuffer>(dst.GetHandle()), 1, &bufferCopy);
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
	auto frameIdx = RendererContext::GetSwapchain()->GetFrameIndex();
	if (mHandle->renderPassState.swapchainTarget)
	{
		VkSemaphore waitSemaphore = As<VkSemaphore>(RendererContext::GetSwapchain()->GetImageAcquireSemaphore());
		VkSemaphore signalSemaphore = As<VkSemaphore>(RendererContext::GetSwapchain()->GetImageReleaseSemaphore());

		VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = &waitSemaphore;
		submitInfo.pWaitDstStageMask = &waitDstStageMask;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &CurrentCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &signalSemaphore;
		VK_CHECK(vkQueueSubmit(As<VkQueue>(RendererContext::GetGraphicsQueue()), 1, &submitInfo, mHandle->fences[frameIdx]));

		RendererContext::GetSwapchain()->Present(CurrentCommandBuffer);

		frameIdx = RendererContext::GetSwapchain()->GetFrameIndex();
		VK_CHECK(vkWaitForFences(VulkanDevice, 1, &mHandle->fences[frameIdx], VK_TRUE, UINT64_MAX));
		VK_CHECK(vkResetCommandPool(VulkanDevice, As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(frameIdx)), 0));

		for (auto framebuffer : mHandle->deletionQueue[frameIdx].framebuffers)
		{
			vkDestroyFramebuffer(VulkanDevice, framebuffer, nullptr);
		}
		mHandle->deletionQueue[frameIdx].framebuffers.clear();
	}
	else
	{
		// TODO:
	}
}

void CommandBuffer::WaitUntilCompleted() const
{
	for (auto fence : mHandle->fences)
	{
		VK_CHECK(vkWaitForFences(VulkanDevice, 1, &fence, VK_TRUE, UINT64_MAX));
	}
}

#endif
