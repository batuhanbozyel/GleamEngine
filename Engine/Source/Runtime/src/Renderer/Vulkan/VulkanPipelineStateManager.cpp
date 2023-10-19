#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanPipelineStateManager.h"
#include "VulkanShaderReflect.h"

using namespace Gleam;

static size_t PipelineHasher(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    size_t hash = 0;
    hash_combine(hash, pipelineDesc);
    hash_combine(hash, vertexShader);
    hash_combine(hash, fragmentShader);
    for (const auto& colorAttachment : colorAttachments)
    {
        hash_combine(hash, colorAttachment.format);
    }
    hash_combine(hash, depthAttachment.format);
    hash_combine(hash, sampleCount);
    return hash;
}

static VkSampler CreateVkSampler(VulkanDevice* device, const SamplerState& samplerState)
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

	VK_CHECK(vkCreateSampler(As<VkDevice>(device->GetHandle()), &createInfo, nullptr, &sampler));
	return sampler;
}

void VulkanPipelineStateManager::Init(VulkanDevice* device)
{
	mDevice = device;
    auto samplerSates = SamplerState::GetAllVariations();
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        mSamplerStates[i] = CreateVkSampler(device, samplerSates[i]);
    }
}

void VulkanPipelineStateManager::Destroy()
{
	for (const auto& sampler : mSamplerStates)
	{
        vkDestroySampler(As<VkDevice>(mDevice->GetHandle()), sampler, nullptr);
	}
	Clear();
}

const VulkanGraphicsPipeline* VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, uint32_t sampleCount)
{
    return GetGraphicsPipeline(pipelineDesc, colorAttachments, TextureDescriptor(), vertexShader, fragmentShader, renderPass, sampleCount);
}

const VulkanGraphicsPipeline* VulkanPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, uint32_t sampleCount)
{
    auto key = PipelineHasher(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, sampleCount);
    auto it = mGraphicsPipelineCache.find(key);
    if (it != mGraphicsPipelineCache.end())
    {
        return it->second.get();
    }

    auto pipeline = new VulkanGraphicsPipeline;
    pipeline->vertexShader = vertexShader;
    pipeline->fragmentShader = fragmentShader;
    pipeline->bindPoint = PipelineBindPointToVkPipelineBindPoint(pipelineDesc.bindPoint);
    pipeline->topology = PrimitiveTopologyToMTLPrimitiveType(pipelineDesc.topology);
    pipeline->layout = CreatePipelineLayout(vertexShader, fragmentShader);
    pipeline->handle = CreateGraphicsPipeline(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, renderPass, pipeline->layout, sampleCount);
    mGraphicsPipelineCache.insert(mGraphicsPipelineCache.end(), {key, Scope<VulkanGraphicsPipeline>(pipeline)});
    return pipeline;
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
	for (const auto& pipeline : mGraphicsPipelineCache)
	{
		vkDestroyPipelineLayout(As<VkDevice>(mDevice->GetHandle()), pipeline->layout, nullptr);
		vkDestroyPipeline(As<VkDevice>(mDevice->GetHandle()), pipeline->handle, nullptr);
	}
	mGraphicsPipelineCache.clear();
}

VkPipelineLayout VulkanPipelineStateManager::CreatePipelineLayout(const Shader& vertexShader, const Shader& fragmentShader)
{
    VkPipelineLayout layout;
    // Pipeline layout
    const auto& vertexShaderReflection = vertexShader.GetReflection();
    const auto& fragmentShaderReflection = fragmentShader.GetReflection();
    
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
    VK_CHECK(vkCreatePipelineLayout(As<VkDevice>(mDevice->GetHandle()), &pipelineLayoutCreateInfo, nullptr, &layout));
    return layout;
}

VkPipeline VulkanPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, VkRenderPass renderPass, VkPipelineLayout layout, uint32_t sampleCount)
{
    VkPipeline pipeline;
    
	VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCreateInfo.renderPass = renderPass;
    pipelineCreateInfo.layout = layout;

	// Shader stages
	TArray<VkPipelineShaderStageCreateInfo, 2> shaderStages{};
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = As<VkShaderModule>(vertexShader.GetHandle());
	shaderStages[0].pName = vertexShader.GetEntryPoint().c_str();

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = As<VkShaderModule>(fragmentShader.GetHandle());
	shaderStages[1].pName = fragmentShader.GetEntryPoint().c_str();

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
	multisampleState.rasterizationSamples = GetVkSampleCount(sampleCount);
	pipelineCreateInfo.pMultisampleState = &multisampleState;

	VkPipelineDepthStencilStateCreateInfo depthStencilState{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	if (depthAttachment.IsValid())
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

	TArray<VkPipelineColorBlendAttachmentState> attachmentBlendStates(colorAttachments.size());
	for (uint32_t i = 0; i < colorAttachments.size(); i++)
	{
		const auto& attachment = colorAttachments[i];
		attachmentBlendStates[i].blendEnable = pipelineDesc.blendState.enabled;
		attachmentBlendStates[i].srcColorBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.sourceColorBlendMode);
		attachmentBlendStates[i].dstColorBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.destinationColorBlendMode);
		attachmentBlendStates[i].srcAlphaBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.sourceAlphaBlendMode);
		attachmentBlendStates[i].dstAlphaBlendFactor = BlendModeToVkBlendFactor(pipelineDesc.blendState.destinationAlphaBlendMode);
		attachmentBlendStates[i].colorBlendOp = BlendOpToVkBlendOp(pipelineDesc.blendState.colorBlendOperation);
		attachmentBlendStates[i].alphaBlendOp = BlendOpToVkBlendOp(pipelineDesc.blendState.alphaBlendOperation);
		attachmentBlendStates[i].colorWriteMask = ColorWriteMaskToVkColorComponentFlags(pipelineDesc.blendState.writeMask);
	}
	
	VkPipelineColorBlendStateCreateInfo colorBlendState{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlendState.attachmentCount = static_cast<uint32_t>(attachmentBlendStates.size());
	colorBlendState.pAttachments = attachmentBlendStates.data();
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	VK_CHECK(vkCreateGraphicsPipelines(As<VkDevice>(mDevice->GetHandle()), nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
    return pipeline;
}
#endif
