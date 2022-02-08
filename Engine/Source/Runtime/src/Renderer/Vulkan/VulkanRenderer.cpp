#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Renderer.h"
#include "VulkanUtils.h"

using namespace Gleam;

void Renderer::ClearScreen(const Color& color)
{
	const auto& frame = RendererContext::GetSwapchain()->GetCurrentFrame();

	VkClearValue clearValue{};
	clearValue.color = std::bit_cast<VkClearColorValue>(color);

	VkRenderPassBeginInfo clearPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	clearPassBeginInfo.clearValueCount = 1;
	clearPassBeginInfo.framebuffer = frame.framebuffer;
	clearPassBeginInfo.pClearValues = &clearValue;
	clearPassBeginInfo.renderPass = As<VkRenderPass>(RendererContext::GetSwapchain()->GetRenderPass());
	clearPassBeginInfo.renderArea.extent.width = RendererContext::GetSwapchain()->GetProperties().width;
	clearPassBeginInfo.renderArea.extent.height = RendererContext::GetSwapchain()->GetProperties().height;

	vkCmdBeginRenderPass(frame.commandBuffer, &clearPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdEndRenderPass(frame.commandBuffer);
}

#endif
