#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Shader.h"
#include "VulkanUtils.h"

using namespace Gleam;

static Shader CreateShader(const TString& entryPoint)
{
	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile("Assets/" + entryPoint + ".spv");

	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = shaderCode.size();
	createInfo.pCode = As<uint32_t*>(shaderCode.data());

	VkShaderModule function;
	VK_CHECK(vkCreateShaderModule(VulkanDevice, &createInfo, nullptr, &function));

	Shader shader;
	shader.entryPoint = entryPoint;
	shader.function = function;
	return shader;
}

/************************************************************************/
/*    GraphicsShader                                                    */
/************************************************************************/
GraphicsShader::GraphicsShader(const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
	mVertexShader = CreateShader(vertexEntryPoint);
	mFragmentShader = CreateShader(fragmentEntryPoint);
}

GraphicsShader::~GraphicsShader()
{
	vkDestroyShaderModule(VulkanDevice, As<VkShaderModule>(mVertexShader.function), nullptr);
	vkDestroyShaderModule(VulkanDevice, As<VkShaderModule>(mFragmentShader.function), nullptr);
}

/************************************************************************/
/*    ComputeShader                                                     */
/************************************************************************/
ComputeShader::ComputeShader(const TString& entryPoint)
{
	mShader = CreateShader(entryPoint);
}

ComputeShader::~ComputeShader()
{
	vkDestroyShaderModule(VulkanDevice, As<VkShaderModule>(mShader.function), nullptr);
}

#endif
