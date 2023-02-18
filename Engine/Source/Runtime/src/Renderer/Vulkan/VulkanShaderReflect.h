#pragma once
#include "Renderer/Shader.h"
#include "VulkanDevice.h"

#include <spirv_reflect.h>

#define SPV_CHECK(x) {SpvReflectResult result = (x);\
					  GLEAM_ASSERT(result == SPV_REFLECT_RESULT_SUCCESS, VkResultToString(x));}

static constexpr VkDescriptorType SpvReflectDescriptorTypeToVkDescriptorType(SpvReflectDescriptorType descriptorType)
{
	switch (descriptorType)
	{
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE: return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
		case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
		case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		default: return VK_DESCRIPTOR_TYPE_MAX_ENUM;
	}
}

static constexpr VkShaderStageFlagBits SpvReflectShaderStageToVkShaderStage(SpvReflectShaderStageFlagBits shaderStage)
{
	switch (shaderStage)
	{
		case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
		case SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT: return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
		case SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
		case SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT: return VK_SHADER_STAGE_GEOMETRY_BIT;
		case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
		case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
		case SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV: return VK_SHADER_STAGE_TASK_BIT_NV;
		case SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV: return VK_SHADER_STAGE_MESH_BIT_NV;
		case SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR: return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR: return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR: return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR: return VK_SHADER_STAGE_MISS_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR: return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
		case SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR: return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
		default: return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
	}
}

namespace Gleam {

struct Shader::Reflection
{
	TArray<VkDescriptorSetLayout> setLayouts;
	TArray<VkPushConstantRange> pushConstantRanges;

	Shader::Reflection(const TArray<uint8_t>& source)
	{
		SpvReflectShaderModule shaderReflection;
		SPV_CHECK(spvReflectCreateShaderModule(source.size(), source.data(), &shaderReflection));

		// Get descriptor sets
		uint32_t setCount = 0;
		spvReflectEnumerateDescriptorSets(&shaderReflection, &setCount, nullptr);

		TArray<SpvReflectDescriptorSet*> descriptorSets(setCount);
		spvReflectEnumerateDescriptorSets(&shaderReflection, &setCount, descriptorSets.data());

		setLayouts.resize(setCount);
		for (uint32_t i = 0; i < setCount; i++)
		{
			const auto& set = descriptorSets[i];

			TArray<VkDescriptorSetLayoutBinding> setBindings(set->binding_count);
			for (uint32_t j = 0; j < set->binding_count; j++)
			{
				auto setBinding = set->bindings[j];
				setBindings[j].binding = setBinding->binding;
				setBindings[j].descriptorType = SpvReflectDescriptorTypeToVkDescriptorType(setBinding->descriptor_type);
				setBindings[j].descriptorCount = setBinding->count;
				setBindings[j].stageFlags = SpvReflectShaderStageToVkShaderStage(shaderReflection.shader_stage);
			}

			VkDescriptorSetLayoutCreateInfo setCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			setCreateInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
			setCreateInfo.pBindings = setBindings.data();
			VK_CHECK(vkCreateDescriptorSetLayout(VulkanDevice::GetHandle(), &setCreateInfo, nullptr, &setLayouts[i]));
		}

		// Get push constant
		uint32_t pushConstantCount = 0;
		spvReflectEnumeratePushConstantBlocks(&shaderReflection, &pushConstantCount, nullptr);

		TArray<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
		spvReflectEnumeratePushConstantBlocks(&shaderReflection, &pushConstantCount, pushConstants.data());

		pushConstantRanges.resize(pushConstantCount);
		for (uint32_t i = 0; i < pushConstantCount; i++)
		{
			const auto& pushConstant = pushConstants[i];
			pushConstantRanges[i].stageFlags = SpvReflectShaderStageToVkShaderStage(shaderReflection.shader_stage);
			pushConstantRanges[i].size = pushConstant->size;
			pushConstantRanges[i].offset = pushConstant->offset;
		}
	}

	~Reflection()
	{
		for (auto setLayout : setLayouts)
		{
			vkDestroyDescriptorSetLayout(VulkanDevice::GetHandle(), setLayout, nullptr);
		}
	}
};

} // namespace Gleam
