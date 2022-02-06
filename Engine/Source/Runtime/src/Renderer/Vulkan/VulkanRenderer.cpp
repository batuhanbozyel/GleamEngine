#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Renderer.h"
#include "VulkanUtils.h"

#include "Renderer/ShaderLibrary.h"

using namespace Gleam;

VkPipeline pipeline = VK_NULL_HANDLE;

void Renderer::BeginFrame()
{
    const auto& frame = RendererContext::GetSwapchain()->AcquireNextFrame();
}

void Renderer::EndFrame()
{
	const auto& frame = RendererContext::GetSwapchain()->GetCurrentFrame();

	VkClearValue clearColor{ 0.1f, 0.1f, 0.1f, 1.0f };
	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = frame.swapchainRenderPass;
	renderPassBeginInfo.framebuffer = frame.framebuffer;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;
	renderPassBeginInfo.renderArea.extent.width = RendererContext::GetProperties().width;
	renderPassBeginInfo.renderArea.extent.height = RendererContext::GetProperties().height;
	vkCmdBeginRenderPass(frame.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	if (pipeline == VK_NULL_HANDLE)
	{
		// Shader stages
		const auto& shader = ShaderLibrary::GetBuiltinGraphicsShader(FullscreenTriangleShader);

		TArray<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
		shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		shaderStages[0].module = As<VkShaderModule>(shader->GetVertexShader().function);
		shaderStages[0].pName = shader->GetVertexShader().entryPoint.c_str();

		shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		shaderStages[1].module = As<VkShaderModule>(shader->GetFragmentShader().function);
		shaderStages[1].pName = shader->GetFragmentShader().entryPoint.c_str();

		pipelineCreateInfo.stageCount = 2;
		pipelineCreateInfo.pStages = shaderStages.data();

		// Input assembly state
		VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		pipelineCreateInfo.pVertexInputState = &vertexInputState;

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

		VkViewport viewport{};
		viewport.width = RendererContext::GetProperties().width;
		viewport.height = RendererContext::GetProperties().height;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.extent.width = RendererContext::GetProperties().width;
		scissor.extent.height = RendererContext::GetProperties().height;

		VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;
		pipelineCreateInfo.pViewportState = &viewportState;

		// Pipeline layout
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		VkPipelineLayout piplineLayout;
		VK_CHECK(vkCreatePipelineLayout(VulkanDevice, &pipelineLayoutCreateInfo, nullptr, &piplineLayout));
		pipelineCreateInfo.layout = piplineLayout;

		VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizationState.lineWidth = 1.0f;
		pipelineCreateInfo.pRasterizationState = &rasterizationState;

		VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisampleState.rasterizationSamples = GetVkSampleCount(RendererContext::GetProperties().msaa);
		pipelineCreateInfo.pMultisampleState = &multisampleState;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
		pipelineCreateInfo.pDepthStencilState = &depthStencilState;

		VkPipelineColorBlendAttachmentState attachmentBlendState{};
		attachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &attachmentBlendState;
		pipelineCreateInfo.pColorBlendState = &colorBlendState;

		pipelineCreateInfo.renderPass = frame.swapchainRenderPass;
		VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
	}
	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

	vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(frame.commandBuffer);

	RendererContext::GetSwapchain()->Present();
}

#endif
