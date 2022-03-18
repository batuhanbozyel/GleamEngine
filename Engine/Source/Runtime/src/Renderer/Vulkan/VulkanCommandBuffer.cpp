#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/CommandBuffer.h"
#include "VulkanUtils.h"

#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"

using namespace Gleam;

CommandBuffer::CommandBuffer()
	: mCommandBuffers(RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight)
{
	for (uint32_t i = 0; i < RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight; i++)
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = As<VkCommandPool>(RendererContext::GetSwapchain()->GetGraphicsCommandPool(i));
		VK_CHECK(vkAllocateCommandBuffers(VulkanDevice, &allocateInfo, As<VkCommandBuffer*>(&mCommandBuffers[i])));
	}
}

CommandBuffer::~CommandBuffer()
{
	for (uint32_t i = 0; i < RendererContext::GetSwapchain()->GetProperties().maxFramesInFlight; i++)
	{
		vkFreeCommandBuffers(VulkanDevice, As<VkCommandPool>(RendererContext::GetSwapchain()->GetGraphicsCommandPool(i)), 1, As<VkCommandBuffer*>(&mCommandBuffers[i]));
	}
}

void CommandBuffer::Begin() const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();

	VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), &beginInfo);
}

void CommandBuffer::End() const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();
	vkEndCommandBuffer(As<VkCommandBuffer>(mCommandBuffers[frameIndex]));
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height) const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();

	VkViewport viewport{};
	viewport.width = width;
	viewport.height = height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = width;
	scissor.extent.height = height;
	vkCmdSetScissor(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), 0, 1, &scissor);
}

void CommandBuffer::BindPipeline(const GraphicsPipeline& pipeline) const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();
	vkCmdBindPipeline(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), VK_PIPELINE_BIND_POINT_GRAPHICS, As<VkPipeline>(pipeline.GetPipeline()));
}

void CommandBuffer::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t baseVertex, uint32_t baseInstance) const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();
	vkCmdDraw(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), vertexCount, instanceCount, baseVertex, baseInstance);
}

void CommandBuffer::DrawIndexed(const IndexBuffer& indexBuffer, uint32_t instanceCount, uint32_t firstIndex, uint32_t baseVertex, uint32_t baseInstance) const
{
	uint32_t frameIndex = RendererContext::GetSwapchain()->GetCurrentFrameIndex();
	vkCmdBindIndexBuffer(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), As<VkBuffer>(indexBuffer.GetBuffer()), 0, static_cast<VkIndexType>(indexBuffer.GetIndexType()));
	vkCmdDrawIndexed(As<VkCommandBuffer>(mCommandBuffers[frameIndex]), indexBuffer.GetCount(), instanceCount, firstIndex, baseVertex, baseInstance);
}

#endif
