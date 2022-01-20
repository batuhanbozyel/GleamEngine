#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Renderer.h"
#include "VulkanUtils.h"

using namespace Gleam;

void Renderer::BeginFrame()
{
    const auto& frame = mContext->AcquireNextFrame();
    
    VkClearValue clearColor{0.0f, 0.0f, 0.0f, 0.0f};
    VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassBeginInfo.renderPass = frame.swapchainRenderPass;
    renderPassBeginInfo.framebuffer = frame.framebuffer;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;
    renderPassBeginInfo.renderArea.extent.width = mContext->GetProperties().width;
    renderPassBeginInfo.renderArea.extent.height = mContext->GetProperties().height;
    vkCmdBeginRenderPass(CurrentFrame.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdEndRenderPass(frame.commandBuffer);
}

void Renderer::EndFrame()
{
    mContext->Present();
}


#endif
