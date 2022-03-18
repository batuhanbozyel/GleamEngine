#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/TileForwardRenderer.h"
#include "VulkanUtils.h"

#include "Renderer/ShaderLibrary.h"

using namespace Gleam;

struct
{
	VkPipeline pipeline;
	VkPipelineLayout piplineLayout;
	VkDescriptorSetLayout setLayout;
} mContext;

TileForwardRenderer::TileForwardRenderer()
	: mVertexBuffer(4), mIndexBuffer(6, IndexType::UINT32)
{
	TArray<Vertex, 4> vertices;
	// top left
	vertices[0].position = { -0.5f, 0.5f, 0.0f };
	vertices[0].texCoord = { 0.0f, 0.0f };
	// top right
	vertices[1].position = { 0.5f, 0.5f, 0.0f };
	vertices[1].texCoord = { 1.0f, 0.0f };
	// bottom right
	vertices[2].position = { 0.5f, -0.5f, 0.0f };
	vertices[2].texCoord = { 1.0f, 1.0f };
	// bottom left
	vertices[3].position = { -0.5f, -0.5f, 0.0f };
	vertices[3].texCoord = { 0.0f, 1.0f };
	mVertexBuffer.SetData(vertices.data(), 0, sizeof(Vertex) * 4);

	TArray<uint32_t, 6> indices
	{
		0, 1, 2,
		2, 3, 0
	};
	mIndexBuffer.SetData(indices.data(), 0, sizeof(uint32_t) * 6);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shader stages
	const auto& program = ShaderLibrary::CreateGraphicsShader("forwardPassVertexShader", "forwardPassFragmentShader");

	TArray<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = As<VkShaderModule>(program.vertexShader->GetHandle());
	shaderStages[0].pName = program.vertexShader->GetEntryPoint().c_str();

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = As<VkShaderModule>(program.fragmentShader->GetHandle());
	shaderStages[1].pName = program.fragmentShader->GetEntryPoint().c_str();

	pipelineCreateInfo.stageCount = 2;
	pipelineCreateInfo.pStages = shaderStages.data();

	// Input assembly state
	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	pipelineCreateInfo.pViewportState = &viewportState;

	// Pipeline layout
	VkDescriptorSetLayoutBinding setBinding{};
	setBinding.binding = 0;
	setBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setBinding.descriptorCount = 1;
	setBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo setCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	setCreateInfo.bindingCount = 1;
	setCreateInfo.pBindings = &setBinding;
	VK_CHECK(vkCreateDescriptorSetLayout(VulkanDevice, &setCreateInfo, nullptr, &mContext.setLayout));

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &mContext.setLayout;
	VK_CHECK(vkCreatePipelineLayout(VulkanDevice, &pipelineLayoutCreateInfo, nullptr, &mContext.piplineLayout));
	pipelineCreateInfo.layout = mContext.piplineLayout;

	// Dynamic state
	TArray<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates.data();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.lineWidth = 1.0f;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = GetVkSampleCount(RendererContext::GetProperties().sampleCount);
	pipelineCreateInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;

	VkPipelineColorBlendAttachmentState attachmentBlendState{};
	attachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &attachmentBlendState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;

	pipelineCreateInfo.renderPass = As<VkRenderPass>(RendererContext::GetSwapchain()->GetRenderPass());
	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &mContext.pipeline));
}

TileForwardRenderer::~TileForwardRenderer()
{
	vkDeviceWaitIdle(VulkanDevice);
	vkDestroyPipeline(VulkanDevice, mContext.pipeline, nullptr);
	vkDestroyPipelineLayout(VulkanDevice, mContext.piplineLayout, nullptr);
	vkDestroyDescriptorSetLayout(VulkanDevice, mContext.setLayout, nullptr);
}

void TileForwardRenderer::Render()
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

	vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext.pipeline);

	VkViewport viewport{};
	viewport.width = RendererContext::GetProperties().width;
	viewport.height = RendererContext::GetProperties().height;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

	VkRect2D scissor{};
	scissor.extent.width = RendererContext::GetProperties().width;
	scissor.extent.height = RendererContext::GetProperties().height;
	vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(mVertexBuffer.GetHandle());
	bufferInfo.range = mVertexBuffer.GetSize();

	VkWriteDescriptorSet descriptor{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptor.descriptorCount = 1;
	descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptor.pBufferInfo = &bufferInfo;

	vkCmdPushDescriptorSetKHR(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mContext.piplineLayout, 0, 1, &descriptor);
	vkCmdBindIndexBuffer(frame.commandBuffer, As<VkBuffer>(mIndexBuffer.GetHandle()), 0, static_cast<VkIndexType>(mIndexBuffer.GetIndexType()));

	vkCmdDrawIndexed(frame.commandBuffer, mIndexBuffer.GetCount(), 1, 0, 0, 0);

	vkCmdEndRenderPass(frame.commandBuffer);
}

#endif