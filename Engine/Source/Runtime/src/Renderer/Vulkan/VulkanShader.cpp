#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "VulkanShaderReflect.h"
#include "VulkanUtils.h"
#include "Renderer/ShaderLibrary.h"
#include "Assets/AssetLibrary.h"

using namespace Gleam;

void ShaderLibrary::Init()
{

}

Shader::Shader(const TString& entryPoint, ShaderStage stage)
	: mEntryPoint(entryPoint), mStage(stage)
{
	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(AssetLibrary::GetDefaultAssetPath().append(entryPoint + ".spv"));

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = As<uint32_t*>(shaderCode.data());

	VK_CHECK(vkCreateShaderModule(VulkanDevice, &createInfo, nullptr, As<VkShaderModule*>(&mHandle)));

	reflection = CreateScope<Reflection>(shaderCode);
}

Shader::~Shader()
{
	vkDestroyShaderModule(VulkanDevice, As<VkShaderModule>(mHandle), nullptr);
}

#endif
