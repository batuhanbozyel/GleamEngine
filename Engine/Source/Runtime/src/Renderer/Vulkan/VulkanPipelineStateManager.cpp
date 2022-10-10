#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanPipelineStateManager.h"
#include "VulkanShaderReflect.h"

#include "Renderer/Renderer.h"

using namespace Gleam;

const VulkanPipeline& VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program, VkRenderPass renderPass)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipelineStateDescriptor == pipelineDesc && element.program == program)
		{
			return element.pipeline;
		}
	}

	GraphicsPipelineCacheElement element;
	element.pipelineStateDescriptor = pipelineDesc;
	element.program = program;
	element.pipeline = CreateGraphicsPipeline(pipelineDesc, program, renderPass);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipeline;
}

void VulkanPipelineStateManager::Clear()
{
	for (const auto& element : mGraphicsPipelineCache)
	{
		vkDestroyPipelineLayout(VulkanDevice, element.pipeline.layout, nullptr);
		vkDestroyPipeline(VulkanDevice, element.pipeline.handle, nullptr);
	}
	mGraphicsPipelineCache.clear();
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
	const auto& vertexShaderReflection = program.vertexShader->GetReflection();
	const auto& fragmentShaderReflection = program.fragmentShader->GetReflection();

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
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
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
