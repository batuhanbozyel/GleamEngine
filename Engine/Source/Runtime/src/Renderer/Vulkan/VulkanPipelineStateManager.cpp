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
		default: GLEAM_ASSERT(false, "Vulkan: Wrap mode is not supported!") break;
	}

	VK_CHECK(vkCreateSampler(static_cast<VkDevice>(device->GetHandle()), &createInfo, nullptr, &sampler));
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
        vkDestroySampler(static_cast<VkDevice>(mDevice->GetHandle()), sampler, nullptr);
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
	pipeline->setLayout = CreateDescriptorSetLayout(vertexShader, fragmentShader);
    pipeline->layout = CreatePipelineLayout(vertexShader, fragmentShader, pipeline->setLayout);
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
	for (const auto& [_, pipeline] : mGraphicsPipelineCache)
	{
		vkDestroyDescriptorSetLayout(static_cast<VkDevice>(mDevice->GetHandle()), pipeline->setLayout, nullptr);
		vkDestroyPipelineLayout(static_cast<VkDevice>(mDevice->GetHandle()), pipeline->layout, nullptr);
		vkDestroyPipeline(static_cast<VkDevice>(mDevice->GetHandle()), pipeline->handle, nullptr);
	}
	mGraphicsPipelineCache.clear();
}

VkDescriptorSetLayout VulkanPipelineStateManager::CreateDescriptorSetLayout(const Shader& vertexShader, const Shader& fragmentShader)
{
	VkDescriptorSetLayout setLayout;

	const auto& vertexShaderReflection = vertexShader.GetReflection();
	const auto& fragmentShaderReflection = fragmentShader.GetReflection();
	auto GetSetBindings = [](const Shader::Reflection* refl)
	{
		TArray<VkDescriptorSetLayoutBinding> setBindings;
		for (const auto& set : refl->descriptorSets)
		{
			size_t setOffset = setBindings.size();
			setBindings.resize(setBindings.size() + set->binding_count);
			for (uint32_t j = 0; j < set->binding_count; j++)
			{
				auto setBinding = set->bindings[j];
				auto bindingIndex = setOffset + j;

				setBindings[bindingIndex].binding = setBinding->binding;
				setBindings[bindingIndex].descriptorType = SpvReflectDescriptorTypeToVkDescriptorType(setBinding->descriptor_type);
				setBindings[bindingIndex].descriptorCount = setBinding->count;
				setBindings[bindingIndex].stageFlags = SpvReflectShaderStageToVkShaderStage(refl->reflection.shader_stage);
			}
		}
		return setBindings;
	};

	TArray<VkDescriptorSetLayoutBinding> vertexBindings = GetSetBindings(vertexShaderReflection);
	TArray<VkDescriptorSetLayoutBinding> fragmentBindings = GetSetBindings(fragmentShaderReflection);

	// Remove duplicate bindings
	std::for_each(vertexBindings.begin(), vertexBindings.end(), [&](VkDescriptorSetLayoutBinding& vertexBinding)
	{
		auto it = std::find_if(fragmentBindings.begin(), fragmentBindings.end(), [&](const VkDescriptorSetLayoutBinding& fragmentBinding)
		{
			return fragmentBinding.binding == vertexBinding.binding;
		});
		if (it != fragmentBindings.end())
		{
			vertexBinding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentBindings.erase(it);
		}
	});

	TArray<VkDescriptorSetLayoutBinding> setBindings;
	setBindings.reserve(vertexBindings.size() + fragmentBindings.size());
	setBindings.insert(setBindings.begin(), vertexBindings.begin(), vertexBindings.end());
	setBindings.insert(setBindings.begin(), fragmentBindings.begin(), fragmentBindings.end());

	VkDescriptorSetLayoutCreateInfo setCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
	setCreateInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
	setCreateInfo.pBindings = setBindings.data();
	VK_CHECK(vkCreateDescriptorSetLayout(static_cast<VkDevice>(mDevice->GetHandle()), &setCreateInfo, nullptr, &setLayout));
	return setLayout;
}

VkPipelineLayout VulkanPipelineStateManager::CreatePipelineLayout(const Shader& vertexShader, const Shader& fragmentShader, VkDescriptorSetLayout setLayout)
{
    VkPipelineLayout layout;
    // Pipeline layout
    const auto& vertexShaderReflection = vertexShader.GetReflection();
    const auto& fragmentShaderReflection = fragmentShader.GetReflection();
    
    // Push constant ranges
    TArray<VkPushConstantRange> pushConstantRanges;
    pushConstantRanges.reserve(vertexShaderReflection->pushConstantRanges.size() + fragmentShaderReflection->pushConstantRanges.size());
    pushConstantRanges.insert(pushConstantRanges.end(), vertexShaderReflection->pushConstantRanges.begin(), vertexShaderReflection->pushConstantRanges.end());
    pushConstantRanges.insert(pushConstantRanges.end(), fragmentShaderReflection->pushConstantRanges.begin(), fragmentShaderReflection->pushConstantRanges.end());
    
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &setLayout;
    pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutCreateInfo.pPushConstantRanges = pushConstantRanges.data();
    VK_CHECK(vkCreatePipelineLayout(static_cast<VkDevice>(mDevice->GetHandle()), &pipelineLayoutCreateInfo, nullptr, &layout));
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
	shaderStages[0].module = static_cast<VkShaderModule>(vertexShader.GetHandle());
	shaderStages[0].pName = vertexShader.GetEntryPoint().c_str();

	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = static_cast<VkShaderModule>(fragmentShader.GetHandle());
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
	if (Utils::IsDepthFormat(depthAttachment.format))
	{
		depthStencilState.depthTestEnable = pipelineDesc.depthState.compareFunction != CompareFunction::Always;
		depthStencilState.depthCompareOp = CompareFunctionToVkCompareOp(pipelineDesc.depthState.compareFunction);
		depthStencilState.depthWriteEnable = pipelineDesc.depthState.writeEnabled;
	}
	if (Utils::IsStencilFormat(depthAttachment.format))
	{
		depthStencilState.stencilTestEnable = pipelineDesc.stencilState.enabled;

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
	VK_CHECK(vkCreateGraphicsPipelines(static_cast<VkDevice>(mDevice->GetHandle()), nullptr, 1, &pipelineCreateInfo, nullptr, &pipeline));
    return pipeline;
}
#endif
