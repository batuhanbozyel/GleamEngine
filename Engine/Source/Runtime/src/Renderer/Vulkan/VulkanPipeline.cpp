#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Pipeline.h"
#include "VulkanUtils.h"

#include "Renderer/ShaderLibrary.h"

using namespace Gleam;

struct
{
	VkPipelineCache vulkanPipelineCache;
	uint32_t pipelineCount = 0;
} mContext;

/************************************************************************/
/*    CreateGraphicsPipeline                                            */
/************************************************************************/
GraphicsPipeline::GraphicsPipeline(const PipelineStateDescriptor& pipelineStateDescriptor, const Ref<GraphicsShader>& shader)
{
	static std::once_flag flag;
	std::call_once(flag, []()
	{
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
		VK_CHECK(vkCreatePipelineCache(VulkanDevice, &pipelineCacheCreateInfo, nullptr, &mContext.vulkanPipelineCache));
	});

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

	// Shader stages
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

	// Vertex input state
	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	// Input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = PrimitiveToplogyToVkPrimitiveTopology(pipelineStateDescriptor.topology);
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;

	// Viewport state: will be set dynamically by calling vkCmdSetViewport...
	VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;
	pipelineCreateInfo.pViewportState = &viewportState;

	// Pipeline layout
	VkDescriptorSetLayoutCreateInfo descriptorSet{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	VkDescriptorSetLayoutCreateFlags       flags;
	uint32_t                               bindingCount;
	const VkDescriptorSetLayoutBinding* pBindings;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	uint32_t                        setLayoutCount;
	const VkDescriptorSetLayout* pSetLayouts;
	uint32_t                        pushConstantRangeCount;
	const VkPushConstantRange* pPushConstantRanges;

	VK_CHECK(vkCreatePipelineLayout(VulkanDevice, &pipelineLayoutCreateInfo, nullptr, As<VkPipelineLayout*>(&mLayout)));
	pipelineCreateInfo.layout = As<VkPipelineLayout>(mLayout);

	// Dynamic state
	TArray<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates.data();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizationState.cullMode = CullModeToVkCullMode(pipelineStateDescriptor.cullingMode);
	rasterizationState.lineWidth = 1.0f;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;

	// TODO:
	const VkPipelineMultisampleStateCreateInfo* pMultisampleState;
	const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
	const VkPipelineColorBlendStateCreateInfo* pColorBlendState;
	VkRenderPass                                     renderPass;
	uint32_t                                         subpass;


	// TODO: Remove this when render passes are implemented
	pipelineCreateInfo.renderPass = As<VkRenderPass>(RendererContext::GetSwapchain()->GetRenderPass());

	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, As<VkPipeline*>(&mPipeline)));

	mContext.pipelineCount++;
}
/************************************************************************/
/*    Flush                                                             */
/************************************************************************/
GraphicsPipeline::~GraphicsPipeline()
{
	vkDestroyPipeline(VulkanDevice, As<VkPipeline>(mPipeline), nullptr);
	vkDestroyPipelineLayout(VulkanDevice, As<VkPipelineLayout>(mLayout), nullptr);

	mContext.pipelineCount--;
	if (mContext.pipelineCount == 0)
	{
		vkDestroyPipelineCache(VulkanDevice, mContext.vulkanPipelineCache, nullptr);
	}
}

#endif