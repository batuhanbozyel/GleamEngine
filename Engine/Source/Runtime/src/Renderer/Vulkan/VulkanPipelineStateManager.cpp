#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanPipelineStateManager.h"
#include "VulkanShaderReflect.h"

using namespace Gleam;

static VkSampler CreateVkSampler(const SamplerState& samplerState)
{
	VkSampler sampler;
	VkSamplerCreateInfo createInfo{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };

	switch (samplerState.filterMode)
	{
		case FilterMode::Point:
		{
			createInfo.minFilter = VK_FILTER_NEAREST;
			createInfo.magFilter = VK_FILTER_NEAREST;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		}
		case FilterMode::Bilinear:
		{
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
			break;
		}
		case FilterMode::Trilinear:
		{
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			break;
		}
		default: GLEAM_ASSERT(false, "Vulkan: Filter mode is not supported!") break;
	}

	switch (samplerState.wrapMode)
	{
		case WrapMode::Repeat:
		{
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			break;
		}
		case WrapMode::Clamp:
		{
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			break;
		}
		case WrapMode::Mirror:
		{
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			break;
		}
		case WrapMode::MirrorOnce:
		{
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
			break;
		}
		default: GLEAM_ASSERT(false, "Vulkan: Filter mode is not supported!") break;
	}

	VK_CHECK(vkCreateSampler(VulkanDevice::GetHandle(), &createInfo, nullptr, &sampler));
	return sampler;
}

void VulkanPipelineStateManager::Init()
{
    auto samplerSates = SamplerState::GetAllVariations();
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        mSamplerStates[i] = CreateVkSampler(samplerSates[i]);
    }
}

void VulkanPipelineStateManager::Destroy()
{
	for (const auto& sampler : mSamplerStates)
	{
        vkDestroySampler(VulkanDevice::GetHandle(), sampler, nullptr);
	}
	Clear();
}

const VulkanGraphicsPipeline* VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
																			  const TArray<TextureDescriptor>& colorAttachments,
																			  const RefCounted<Shader>& vertexShader,
																			  const RefCounted<Shader>& fragmentShader,
																			  VkRenderPass renderPass,
																			  uint32_t sampleCount)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipeline.descriptor == pipelineDesc &&
			element.pipeline.vertexShader == vertexShader &&
			element.pipeline.fragmentShader == fragmentShader &&
			element.colorAttachments.size() == colorAttachments.size() &&
			element.sampleCount == sampleCount &&
			!element.hasDepthAttachment)
		{
			bool found = true;
			for (uint32_t i = 0; i < colorAttachments.size(); i++)
			{
				if (element.colorAttachments[i] != colorAttachments[i])
				{
					found = false;
					break;
				}
			}

			if (found) { return &element.pipeline; }
		}
	}

	GraphicsPipelineCacheElement element;
	element.sampleCount = sampleCount;
	element.colorAttachments = colorAttachments;
	element.pipeline.descriptor = pipelineDesc;
	element.pipeline.vertexShader = vertexShader;
	element.pipeline.fragmentShader = fragmentShader;
	CreateGraphicsPipeline(element, renderPass);
	
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return &cachedElement.pipeline;
}

const VulkanGraphicsPipeline* VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc,
																	  const TArray<TextureDescriptor>& colorAttachments,
																	  const TextureDescriptor& depthAttachment,
																	  const RefCounted<Shader>& vertexShader,
																	  const RefCounted<Shader>& fragmentShader,
																	  VkRenderPass renderPass,
																	  uint32_t sampleCount)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipeline.descriptor == pipelineDesc &&
			element.pipeline.vertexShader == vertexShader &&
			element.pipeline.fragmentShader == fragmentShader &&
			element.colorAttachments.size() == colorAttachments.size() &&
			element.depthAttachment == depthAttachment &&
			element.sampleCount == sampleCount &&
			element.hasDepthAttachment)
		{
			bool found = true;
			for (uint32_t i = 0; i < colorAttachments.size(); i++)
			{
				if (element.colorAttachments[i] != colorAttachments[i])
				{
					found = false;
					break;
				}
			}

			if (found) { return &element.pipeline; }
		}
	}

	GraphicsPipelineCacheElement element;
	element.hasDepthAttachment = true;
	element.sampleCount = sampleCount;
	element.colorAttachments = colorAttachments;
	element.depthAttachment = depthAttachment;
	element.pipeline.descriptor = pipelineDesc;
	element.pipeline.vertexShader = vertexShader;
	element.pipeline.fragmentShader = fragmentShader;
	CreateGraphicsPipeline(element, renderPass);

	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return &cachedElement.pipeline;
}

VkSampler VulkanPipelineStateManager::GetSampler(uint32_t index)
{
	return mSamplerStates[index];
}

VkSampler VulkanPipelineStateManager::GetSampler(const SamplerState& samplerState)
{
    std::hash<SamplerState> hasher;
    return mSamplerStates[hasher(samplerState)];
}

void VulkanPipelineStateManager::Clear()
{
	for (const auto& element : mGraphicsPipelineCache)
	{
		vkDestroyPipelineLayout(VulkanDevice::GetHandle(), element.pipeline.layout, nullptr);
		vkDestroyPipeline(VulkanDevice::GetHandle(), element.pipeline.handle, nullptr);
	}
	mGraphicsPipelineCache.clear();
}

void VulkanPipelineStateManager::CreateGraphicsPipeline(GraphicsPipelineCacheElement& element, VkRenderPass renderPass)
{
	const auto& pipelineDesc = element.pipeline.descriptor;
	element.pipeline.bindPoint = PipelineBindPointToVkPipelineBindPoint(pipelineDesc.bindPoint);

	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.renderPass = renderPass;

	// Shader stages
	TArray<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = As<VkShaderModule>(element.pipeline.vertexShader->GetHandle());
	shaderStages[0].pName = element.pipeline.vertexShader->GetEntryPoint().c_str();

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = As<VkShaderModule>(element.pipeline.fragmentShader->GetHandle());
	shaderStages[1].pName = element.pipeline.fragmentShader->GetEntryPoint().c_str();

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
	const auto& vertexShaderReflection = element.pipeline.vertexShader->GetReflection();
	const auto& fragmentShaderReflection = element.pipeline.fragmentShader->GetReflection();

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

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
	pipelineLayoutCreateInfo.pSetLayouts = setLayouts.data();
	pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
	pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
	VK_CHECK(vkCreatePipelineLayout(VulkanDevice::GetHandle(), &pipelineLayoutCreateInfo, nullptr, &element.pipeline.layout));
	pipelineCreateInfo.layout = element.pipeline.layout;

	// Dynamic state
	TArray<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates.data();
	pipelineCreateInfo.pDynamicState = &dynamicState;

	VkPipelineRasterizationStateCreateInfo rasterizationState{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.cullMode = CullModeToVkCullMode(pipelineDesc.cullingMode);
	rasterizationState.lineWidth = 1.0f;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;

	VkPipelineMultisampleStateCreateInfo multisampleState{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampleState.rasterizationSamples = GetVkSampleCount(element.sampleCount);
	pipelineCreateInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if (element.hasDepthAttachment)
	{
		depthStencilState.depthTestEnable = true;
		depthStencilState.depthCompareOp = CompareFunctionToVkCompareOp(pipelineDesc.depthState.compareFunction);
		depthStencilState.depthWriteEnable = pipelineDesc.depthState.writeEnabled;
		
		if (pipelineDesc.stencilState.enabled)
		{
			depthStencilState.stencilTestEnable = true;

			VkStencilOpState stencilState{};
			stencilState.reference = pipelineDesc.stencilState.reference;
			stencilState.compareMask = pipelineDesc.stencilState.readMask;
			stencilState.writeMask = pipelineDesc.stencilState.writeMask;
			stencilState.failOp = StencilOpToVkStencilOp(pipelineDesc.stencilState.failOperation);
			stencilState.passOp = StencilOpToVkStencilOp(pipelineDesc.stencilState.passOperation);
			stencilState.depthFailOp = StencilOpToVkStencilOp(pipelineDesc.stencilState.depthFailOperation);
			stencilState.compareOp = CompareFunctionToVkCompareOp(pipelineDesc.stencilState.compareFunction);

			depthStencilState.back = stencilState;
			depthStencilState.front = stencilState;
		}
	}
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;

	TArray<VkPipelineColorBlendAttachmentState> attachmentBlendStates(element.colorAttachments.size());
	for (uint32_t i = 0; i < element.colorAttachments.size(); i++)
	{
		const auto& attachment = element.colorAttachments[i];
		attachmentBlendStates[i].blendEnable = element.pipeline.descriptor.blendState.enabled;
		attachmentBlendStates[i].srcColorBlendFactor = BlendModeToVkBlendFactor(element.pipeline.descriptor.blendState.sourceColorBlendMode);
		attachmentBlendStates[i].dstColorBlendFactor = BlendModeToVkBlendFactor(element.pipeline.descriptor.blendState.destinationColorBlendMode);
		attachmentBlendStates[i].srcAlphaBlendFactor = BlendModeToVkBlendFactor(element.pipeline.descriptor.blendState.sourceAlphaBlendMode);
		attachmentBlendStates[i].dstAlphaBlendFactor = BlendModeToVkBlendFactor(element.pipeline.descriptor.blendState.destinationAlphaBlendMode);
		attachmentBlendStates[i].colorBlendOp = BlendOpToVkBlendOp(element.pipeline.descriptor.blendState.colorBlendOperation);
		attachmentBlendStates[i].alphaBlendOp = BlendOpToVkBlendOp(element.pipeline.descriptor.blendState.alphaBlendOperation);
		attachmentBlendStates[i].colorWriteMask = ColorWriteMaskToVkColorComponentFlags(element.pipeline.descriptor.blendState.writeMask);
	}
	
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = static_cast<uint32_t>(attachmentBlendStates.size());
	colorBlendState.pAttachments = attachmentBlendStates.data();
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	VK_CHECK(vkCreateGraphicsPipelines(VulkanDevice::GetHandle(), nullptr, 1, &pipelineCreateInfo, nullptr, &element.pipeline.handle));
}
#endif
