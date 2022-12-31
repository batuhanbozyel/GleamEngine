#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanShaderReflect.h"
#include "VulkanUtils.h"

#include "Core/Application.h"

using namespace Gleam;

Shader::Shader(const TString& entryPoint, ShaderStage stage)
	: mEntryPoint(entryPoint), mStage(stage)
{
	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(ApplicationInstance.GetDefaultAssetPath().append(entryPoint + ".spv"));

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = As<uint32_t*>(shaderCode.data());

	VK_CHECK(vkCreateShaderModule(VulkanDevice::GetHandle(), &createInfo, nullptr, As<VkShaderModule*>(&mHandle)));

	mReflection = CreateScope<Reflection>(shaderCode);
}

Shader::~Shader()
{
	vkDestroyShaderModule(VulkanDevice::GetHandle(), As<VkShaderModule>(mHandle), nullptr);
}

#endif
