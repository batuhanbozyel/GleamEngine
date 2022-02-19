#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Shader.h"
#include "VulkanUtils.h"

using namespace Gleam;

Shader::Shader(const TString& entryPoint)
	: mEntryPoint(entryPoint)
{
	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile("Assets/" + entryPoint + ".spv");

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = As<uint32_t*>(shaderCode.data());

	VK_CHECK(vkCreateShaderModule(VulkanDevice, &createInfo, nullptr, As<VkShaderModule*>(&mFunction)));
}

Shader::~Shader()
{
	vkDestroyShaderModule(VulkanDevice, As<VkShaderModule>(mFunction), nullptr);
}
#endif
