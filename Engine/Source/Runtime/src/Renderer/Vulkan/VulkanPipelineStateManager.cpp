#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanPipelineStateManager.h"
#include "VulkanShaderReflect.h"

#include "Renderer/Renderer.h"

using namespace Gleam;

const VulkanRenderPass& VulkanPipelineStateManager::GetRenderPass(const RenderPassDescriptor& renderPassDesc)
{
	for (uint32_t i = 0; i < mRenderPassCache.size(); i++)
	{
		const auto& element = mRenderPassCache[i];
		if (element.renderPassDescriptor == renderPassDesc)
		{
			return element.renderPass;
		}
	}

	RenderPassCacheElement element;
	element.renderPassDescriptor = renderPassDesc;
	element.renderPass.handle = CreateRenderPass(renderPassDesc);
	element.renderPass.swapchainTarget = renderPassDesc.swapchainTarget;
	const auto& cachedElement = mRenderPassCache.emplace_back(element);
	return cachedElement.renderPass;
}

const VulkanPipeline& VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipelineStateDescriptor == pipelineDesc && element.program == program && element.renderPass == renderPass)
		{
			return element.pipeline;
		}
	}

	GraphicsPipelineCacheElement element;
	element.pipelineStateDescriptor = pipelineDesc;
	element.program = program;
	element.renderPass = renderPass;
	element.pipeline = CreateGraphicsPipeline(pipelineDesc, program, renderPass);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipeline;
}

void VulkanPipelineStateManager::Clear()
{
	for (const auto& element : mRenderPassCache)
	{
		vkDestroyRenderPass(VulkanDevice, element.renderPass.handle, nullptr);
	}
	mRenderPassCache.clear();

	for (const auto& element : mGraphicsPipelineCache)
	{
		vkDestroyPipelineLayout(VulkanDevice, element.pipeline.layout, nullptr);
		vkDestroyPipeline(VulkanDevice, element.pipeline.handle, nullptr);
	}
	mGraphicsPipelineCache.clear();
}

VkRenderPass VulkanPipelineStateManager::CreateRenderPass(const RenderPassDescriptor& renderPassDesc)
{
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
	}
	else
	{
		// TODO:
	}

	VkRenderPass renderPass;
	VkRenderPassCreateInfo renderPassCreateInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
	renderPassCreateInfo.attachmentCount = attachmentDescriptors.size();
	renderPassCreateInfo.pAttachments = attachmentDescriptors.data();
	renderPassCreateInfo.subpassCount = subpassDescriptors.size();
	renderPassCreateInfo.pSubpasses = subpassDescriptors.data();
	renderPassCreateInfo.dependencyCount = subpassDependencies.size();
	renderPassCreateInfo.pDependencies = subpassDependencies.data();
	VK_CHECK(vkCreateRenderPass(VulkanDevice, &renderPassCreateInfo, nullptr, &renderPass));

	return renderPass;
}

VulkanPipeline VulkanPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass)
{
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
	inputAssemblyState.topology = PrimitiveToplogyToVkPrimitiveTopology(pipelineDesc.topology);
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	pipelineCreateInfo.pViewportState = &viewportState;

	// Pipeline layout
	const auto& vertexShaderReflection = program.vertexShader->reflection;
	const auto& fragmentShaderReflection = program.fragmentShader->reflection;

	// Descriptor set layouts
	TArray<VkDescriptorSetLayout> setLayouts;
	setLayouts.reserve(vertexShaderReflection->setLayouts.size() + fragmentShaderReflection->setLayouts.size());
	setLayouts.insert(setLayouts.end(), vertexShaderReflection->setLayouts.begin(), vertexShaderReflection->setLayouts.end());
	setLayouts.insert(setLayouts.end(), fragmentShaderReflection->setLayouts.begin(), fragmentShaderReflection->setLayouts.end());

	// Push constant ranges
	TArray<VkPushConstantRange> pushConstantRanges;
	pushConstantRanges.reserve(vertexShaderReflection->pushConstantRanges.size() + fragmentShaderReflection->pushConstantRanges.size());
	pushConstantRanges.insert(pushConstantRanges.end(), vertexShaderReflection->pushConstantRanges.begin(), vertexShaderReflection->pushConstantRanges.end());
	pushConstantRanges.insert(pushConstantRanges.end(), fragmentShaderReflection->pushConstantRanges.begin(), fragmentShaderReflection->pushConstantRanges.end());

	VkPipelineLayout pipelineLayout;
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = setLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = pushConstantRanges.size();
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
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

	pipelineCreateInfo.renderPass = renderPass;
	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));

	return VulkanPipeline
	{
		pipeline,
		pipelineLayout
	};
}
#endif
