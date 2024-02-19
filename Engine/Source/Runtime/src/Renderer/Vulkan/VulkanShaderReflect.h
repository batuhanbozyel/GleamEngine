#pragma once
#include "Renderer/Shader.h"
#include "VulkanDevice.h"
#include "VulkanUtils.h"

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
	static constexpr uint32_t SRV_BINDING_OFFSET = 1000;
	static constexpr uint32_t UAV_BINDING_OFFSET = 2000;

	const GraphicsDevice* device;

	SpvReflectShaderModule reflection;
	TArray<VkDescriptorSetLayout> setLayouts;
	TArray<VkPushConstantRange> pushConstantRanges;

	TArray<SpvReflectDescriptorBinding*> samplers;
	TArray<SpvReflectDescriptorBinding*> SRVs;
	TArray<SpvReflectDescriptorBinding*> CBVs;
	TArray<SpvReflectDescriptorBinding*> UAVs;

	Reflection(const GraphicsDevice* device, const TArray<uint8_t>& source)
		: device(device)
	{
		SPV_CHECK(spvReflectCreateShaderModule(source.size(), source.data(), &reflection));

		// Get descriptor sets
		uint32_t setCount = 0;
		spvReflectEnumerateDescriptorSets(&reflection, &setCount, nullptr);

		TArray<SpvReflectDescriptorSet*> descriptorSets(setCount);
		spvReflectEnumerateDescriptorSets(&reflection, &setCount, descriptorSets.data());

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
				setBindings[j].stageFlags = SpvReflectShaderStageToVkShaderStage(reflection.shader_stage);

				if (setBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_SAMPLER)
				{
					samplers.push_back(setBinding);
				}
				else if (setBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_CBV)
				{
					CBVs.push_back(setBinding);
				}
				else if (setBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_SRV)
				{
					SRVs.push_back(setBinding);
				}
				else if (setBinding->resource_type & SPV_REFLECT_RESOURCE_FLAG_UAV)
				{
					UAVs.push_back(setBinding);
				}
			}

			VkDescriptorSetLayoutCreateInfo setCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			setCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
			setCreateInfo.bindingCount = static_cast<uint32_t>(setBindings.size());
			setCreateInfo.pBindings = setBindings.data();
			VK_CHECK(vkCreateDescriptorSetLayout(As<VkDevice>(device->GetHandle()), &setCreateInfo, nullptr, &setLayouts[i]));
		}

		// Get push constant
		uint32_t pushConstantCount = 0;
		spvReflectEnumeratePushConstantBlocks(&reflection, &pushConstantCount, nullptr);

		TArray<SpvReflectBlockVariable*> pushConstants(pushConstantCount);
		spvReflectEnumeratePushConstantBlocks(&reflection, &pushConstantCount, pushConstants.data());

		pushConstantRanges.resize(pushConstantCount);
		for (uint32_t i = 0; i < pushConstantCount; i++)
		{
			const auto& pushConstant = pushConstants[i];
			pushConstantRanges[i].stageFlags = SpvReflectShaderStageToVkShaderStage(reflection.shader_stage);
			pushConstantRanges[i].size = pushConstant->size;
			pushConstantRanges[i].offset = pushConstant->offset;
		}

		auto comparator = [](const SpvReflectDescriptorBinding* left, const SpvReflectDescriptorBinding* right)
		{
			return left->binding < right->binding;
		};

		std::sort(SRVs.begin(), SRVs.end(), comparator);
		std::sort(CBVs.begin(), CBVs.end(), comparator);
		std::sort(UAVs.begin(), UAVs.end(), comparator);
	}

	~Reflection()
	{
		spvReflectDestroyShaderModule(&reflection);
		for (auto setLayout : setLayouts)
		{
			vkDestroyDescriptorSetLayout(As<VkDevice>(device->GetHandle()), setLayout, nullptr);
		}
	}

	static const SpvReflectDescriptorBinding* GetResourceFromTypeArray(const TArray<SpvReflectDescriptorBinding*>& resources, uint32_t slot)
	{
		if (resources.empty())
		{
			GLEAM_ASSERT(false, "Vulkan: Shader does not use any resource");
			return nullptr;
		}

		uint32_t left = 0;
		uint32_t right = static_cast<uint32_t>(resources.size() - 1);
		while (left <= right)
		{
			uint32_t middle = (right + left) / 2;

			const auto& resource = resources[middle];
			if (resource->binding == slot) { return resource; }

			if (resource->binding < slot) { left = middle + 1; }
			else { right = middle - 1; }
		}

		GLEAM_ASSERT(false, "Vulkan: Resource reflection not found in slot {0}", slot);
		return nullptr;
	}
};

} // namespace Gleam
