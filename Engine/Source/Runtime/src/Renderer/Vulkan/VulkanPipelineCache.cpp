#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/PipelineCache.h"
#include "VulkanUtils.h"

#include "Renderer/ShaderLibrary.h"

using namespace Gleam;

VkPipelineCache mVulkanPipelineCache;

/************************************************************************/
/*    Init                                                              */
/************************************************************************/
void PipelineCache::Init()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
	VK_CHECK(vkCreatePipelineCache(VulkanDevice, &pipelineCacheCreateInfo, nullptr, &mVulkanPipelineCache));
}
/************************************************************************/
/*    Destroy                                                           */
/************************************************************************/
void PipelineCache::Destroy()
{
	Flush();
	vkDestroyPipelineCache(VulkanDevice, mVulkanPipelineCache, nullptr);
}
/************************************************************************/
/*    CreateGraphicsPipeline                                            */
/************************************************************************/
NativeGraphicsHandle PipelineCache::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineStateDescriptor, const TArray<VertexLayoutDescriptor>& vertexDescriptors, const Ref<GraphicsShader>& shader, PrimitiveTopology topology)
{
// 	auto cacheIt = mGraphicsPipelineCache.find(pipelineStateDescriptor);
// 	if (cacheIt != mGraphicsPipelineCache.end())
// 	{
// 		return cacheIt->second;
// 	}

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
	TArray<VkVertexInputBindingDescription> inputBindingDescriptions(vertexDescriptors.size());
	TArray<VkVertexInputAttributeDescription> inputAttributeDescriptions;
	for (uint32_t i = 0; i < inputBindingDescriptions.size(); i++)
	{
		inputBindingDescriptions[i].binding = i;
		inputBindingDescriptions[i].stride = vertexDescriptors[i].stride;
		inputBindingDescriptions[i].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		for (uint32_t j = 0; j < vertexDescriptors[j].attributes.size(); j++)
		{
			VkVertexInputAttributeDescription inputAttributeDescription{};
			inputAttributeDescription.location = j;
			inputAttributeDescription.binding = i;
			inputAttributeDescription.format = VertexAttributeFormatToVkFormat(vertexDescriptors[i].attributes[j].format);
			inputAttributeDescription.offset = vertexDescriptors[i].attributes[j].offset;
			inputAttributeDescriptions.push_back(inputAttributeDescription);
		}
	}

	VkPipelineVertexInputStateCreateInfo vertexInputState{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(inputBindingDescriptions.size());
	vertexInputState.pVertexBindingDescriptions = inputBindingDescriptions.data();
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(inputAttributeDescriptions.size());
	vertexInputState.pVertexAttributeDescriptions = inputAttributeDescriptions.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputState;

	// Input assembly state
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyState.topology = PrimitiveToplogyToVkPrimitiveTopology(topology);
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

	VkPipelineLayout piplineLayout;
	VK_CHECK(vkCreatePipelineLayout(VulkanDevice, &pipelineLayoutCreateInfo, nullptr, &piplineLayout));
	pipelineCreateInfo.layout = piplineLayout;

	// Dynamic state
	TArray<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates.data();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	VkBool32                                   depthClampEnable;
	VkBool32                                   rasterizerDiscardEnable;
	VkPolygonMode                              polygonMode;
	VkCullModeFlags                            cullMode;
	VkFrontFace                                frontFace;
	VkBool32                                   depthBiasEnable;
	float                                      depthBiasConstantFactor;
	float                                      depthBiasClamp;
	float                                      depthBiasSlopeFactor;
	float                                      lineWidth;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;

	// TODO:
	const VkPipelineMultisampleStateCreateInfo* pMultisampleState;
	const VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
	const VkPipelineColorBlendStateCreateInfo* pColorBlendState;
	VkRenderPass                                     renderPass;
	uint32_t                                         subpass;


	VkPipeline pipeline;
	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice, nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
	//mGraphicsPipelineCache.insert({ pipelineStateDescriptor, pipeline });
	return pipeline;
}
/************************************************************************/
/*    Flush                                                             */
/************************************************************************/
void PipelineCache::Flush()
{
	//mGraphicsPipelineCache.clear();
}

#endif