#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/CommandBuffer.h"
#include "VulkanUtils.h"

#include "Renderer/Buffer.h"
#include "Renderer/Renderer.h"
#include "Renderer/ShaderLibrary.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

#define CurrentCommandBuffer (mHandle->commandBuffers[RendererContext::GetSwapchain()->GetCurrentFrameIndex()])
#define ActivePipeline (mHandle->graphicsPipelineCache[mHandle->activePipelineIndex])

using namespace Gleam;

struct GraphicsPipelineCacheElement
{
	RenderPassDescriptor renderPassDescriptor;
	PipelineStateDescriptor pipelineStateDescriptor;
	GraphicsShader program;
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout layout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout setLayout{ VK_NULL_HANDLE };
};

struct CommandBuffer::Impl
{
	TArray<VkCommandBuffer> commandBuffers;
	TArray<GraphicsPipelineCacheElement> graphicsPipelineCache;
	VkCommandBuffer currentCommandBuffer{ VK_NULL_HANDLE };
	uint32_t activePipelineIndex = 0;

	VkRenderPass activeRenderPass{ VK_NULL_HANDLE };
	VkFramebuffer activeFramebuffer{ VK_NULL_HANDLE };
	VkDescriptorSet vertexDescriptorSet{ VK_NULL_HANDLE };
	VkDescriptorSet fragmentDescriptorSet{ VK_NULL_HANDLE };
};

CommandBuffer::CommandBuffer()
	: mHandle(CreateScope<Impl>())
{
	mHandle->commandBuffers.resize(RendererContext::GetProperties().maxFramesInFlight, VK_NULL_HANDLE);
	for (uint32_t i = 0; i < RendererContext::GetProperties().maxFramesInFlight; i++)
	{
		VkCommandBufferAllocateInfo allocateInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		allocateInfo.commandBufferCount = 1;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandPool = As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(i));
		VK_CHECK(vkAllocateCommandBuffers(VulkanDevice, &allocateInfo, &mHandle->commandBuffers[i]));
	}
}

CommandBuffer::~CommandBuffer()
{
	// TODO: Check if VK resources are leaking
	for (const auto& element : mHandle->graphicsPipelineCache)
	{
		vkDestroyDescriptorSetLayout(VulkanDevice, element.setLayout, nullptr);
		vkDestroyPipelineLayout(VulkanDevice, element.layout, nullptr);
		vkDestroyPipeline(VulkanDevice, element.pipeline, nullptr);
	}

	for (uint32_t i = 0; i < RendererContext::GetProperties().maxFramesInFlight; i++)
	{
		vkFreeCommandBuffers(VulkanDevice, As<VkCommandPool>(RendererContext::GetGraphicsCommandPool(i)), 1,&mHandle->commandBuffers[i]);
	}
}

void CommandBuffer::BeginRenderPass(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
	TArray<VkImageView> imageViews(renderPassDesc.attachments.size());
	TArray<VkClearValue> clearValues(renderPassDesc.attachments.size());
	TArray<VkSubpassDescription> subpassDescriptors(renderPassDesc.subpasses.size());
	TArray<VkSubpassDependency> subpassDependencies(renderPassDesc.subpasses.size());
	TArray<VkAttachmentDescription> attachmentDescriptors(renderPassDesc.attachments.size());
	if (renderPassDesc.swapchainTarget)
	{
		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = 0;
		attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpassDesc{};
		subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpassDesc.colorAttachmentCount = 1;
		subpassDesc.pColorAttachments = &attachmentRef;
		subpassDescriptors.emplace_back(subpassDesc);

		VkSubpassDependency subpassDependency{};
		subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpassDependency.dstSubpass = 0;
		subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		subpassDependency.dependencyFlags = 0;
		subpassDependencies.emplace_back(subpassDependency);

		VkAttachmentDescription attachmentDesc{};
		attachmentDesc.format = TextureFormatToVkFormat(RendererContext::GetSwapchain()->GetFormat());
		attachmentDesc.samples = GetVkSampleCount(RendererContext::GetProperties().sampleCount);
		attachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		attachmentDescriptors.emplace_back(attachmentDesc);

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

		imageViews.push_back(As<VkImageView>(RendererContext::GetSwapchain()->AcquireNextDrawable()));
	}
	else
	{
		// TODO:
	}

	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = attachmentDescriptors.size();
	renderPassCreateInfo.pAttachments = attachmentDescriptors.data();
	renderPassCreateInfo.subpassCount = subpassDescriptors.size();
	renderPassCreateInfo.pSubpasses = subpassDescriptors.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VK_CHECK(vkCreateRenderPass(VulkanDevice, &renderPassCreateInfo, nullptr, &mHandle->activeRenderPass));

	VkFramebufferCreateInfo framebufferCreateInfo{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
	framebufferCreateInfo.renderPass = mHandle->activeRenderPass;
	framebufferCreateInfo.attachmentCount = imageViews.size();
	framebufferCreateInfo.pAttachments = imageViews.data();
	framebufferCreateInfo.width = renderPassDesc.width;
	framebufferCreateInfo.height = renderPassDesc.height;
	framebufferCreateInfo.layers = 1;
	VK_CHECK(vkCreateFramebuffer(VulkanDevice, &framebufferCreateInfo, nullptr, &mHandle->activeFramebuffer));

	VkRenderPassBeginInfo renderPassBeginInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
	renderPassBeginInfo.renderPass = mHandle->activeRenderPass;
	renderPassBeginInfo.framebuffer = mHandle->activeFramebuffer;
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.renderArea.extent.width = renderPassDesc.width;
	renderPassBeginInfo.renderArea.extent.height = renderPassDesc.height;
	vkCmdBeginRenderPass(CurrentCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	ApplyRenderPipeline(renderPassDesc, pipelineDesc, program);
}

void CommandBuffer::EndRenderPass() const
{
	vkCmdEndRenderPass(CurrentCommandBuffer);
}

void CommandBuffer::SetViewport(uint32_t width, uint32_t height) const
{
	VkViewport viewport{};
	viewport.width = width;
	viewport.height = height;
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
	bufferInfo.range = buffer.GetSize();

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstSet = mHandle->vertexDescriptorSet;
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(buffer.GetUsage());
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ActivePipeline.layout, 0, 1, &descriptorSet);
}

void CommandBuffer::SetFragmentBuffer(const Buffer& buffer, uint32_t index, uint32_t offset) const
{
	VkDescriptorBufferInfo bufferInfo{};
	bufferInfo.buffer = As<VkBuffer>(buffer.GetHandle());
	bufferInfo.range = buffer.GetSize();

	VkWriteDescriptorSet descriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	descriptorSet.dstSet = mHandle->fragmentDescriptorSet;
	descriptorSet.dstBinding = index;
	descriptorSet.descriptorCount = 1;
	descriptorSet.descriptorType = BufferUsageToVkDescriptorType(buffer.GetUsage());
	descriptorSet.pBufferInfo = &bufferInfo;
	vkCmdPushDescriptorSetKHR(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ActivePipeline.layout, 1, 1, &descriptorSet);
}

void CommandBuffer::SetPushConstant(const void* data, uint32_t size, ShaderStage stage, uint32_t index) const
{
	switch (stage)
	{
		case ShaderStage::Vertex:
		{
			vkCmdPushConstants(CurrentCommandBuffer, ActivePipeline.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, size, data);
			break;
		}
		case ShaderStage::Fragment:
		{
			vkCmdPushConstants(CurrentCommandBuffer, ActivePipeline.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, size, data);
			break;
		}
		case ShaderStage::Compute:
		{
			vkCmdPushConstants(CurrentCommandBuffer, ActivePipeline.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, size, data);
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
	if (ActivePipeline.renderPassDescriptor.swapchainTarget)
	{
		RendererContext::GetSwapchain()->Present(CurrentCommandBuffer);
	}
	else
	{
		// TODO:
	}
}

void CommandBuffer::ApplyRenderPipeline(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program) const
{
	for (uint32_t i = 0; i < mHandle->graphicsPipelineCache.size(); i++)
	{
		const auto& element = mHandle->graphicsPipelineCache[i];
		if (element.renderPassDescriptor == renderPassDesc
			&& element.pipelineStateDescriptor == pipelineDesc
			&& element.program == program)
		{
			mHandle->activePipelineIndex = i;
			vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, element.pipeline);
			return;
		}
	}

	VkPipeline pipeline;
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shader stages
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
	VkDescriptorSetLayout setLayout;
	VkDescriptorSetLayoutBinding setBinding{};
	setBinding.binding = 0;
	setBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	setBinding.descriptorCount = 1;
	setBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayoutCreateInfo setCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	setCreateInfo.bindingCount = 1;
	setCreateInfo.pBindings = &setBinding;
	VK_CHECK(vkCreateDescriptorSetLayout(VulkanDevice, &setCreateInfo, nullptr, &setLayout));

	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
	VK_CHECK(vkCreatePipelineLayout(VulkanDevice, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
	pipelineCreateInfo.layout = pipelineLayout;

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

	pipelineCreateInfo.renderPass = mHandle->activeRenderPass;
	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));

	GraphicsPipelineCacheElement element;
	element.pipelineStateDescriptor = pipelineDesc;
	element.renderPassDescriptor = renderPassDesc;
	element.program = program;
	element.pipeline = pipeline;
	element.layout = pipelineLayout;
	element.setLayout = setLayout;

	mHandle->activePipelineIndex = mHandle->graphicsPipelineCache.size();
	mHandle->graphicsPipelineCache.push_back(element);
	vkCmdBindPipeline(CurrentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, element.pipeline);
}

#endif
