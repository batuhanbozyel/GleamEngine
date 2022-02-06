#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/Shader.h"
#include "VulkanUtils.h"

using namespace Gleam;

static TString GetShaderTargetByType(ShaderType type)
{
    switch(type)
    {
        case ShaderType::Vertex: return "-fvk-invert-y -T vs_6_0";
        case ShaderType::Fragment: return "-T ps_6_0";
        case ShaderType::Compute: return "-T cs_6_0";
        default: GLEAM_ASSERT(false); return "Unknown target";
    }
}

static TString GetSPIRVOutputFile(const TString& filename)
{
    return "Assets/" + filename + ".spv";
}

static bool ShouldGenerateSPIRVForShader(const TString& filename, const TString& entryPoint)
{
    const auto& spirvFilename = GetSPIRVOutputFile(entryPoint);
    if (std::filesystem::exists(spirvFilename))
    {
        const auto& binaryChangeTime = std::filesystem::last_write_time(spirvFilename);
        const auto& sourceChangeTime = std::filesystem::last_write_time(filename);
        return sourceChangeTime > binaryChangeTime;
    }
    return true;
}

static void GenerateSPIRVForTarget(const TString& filename, const TString& entryPoint, ShaderType type)
{
    if (ShouldGenerateSPIRVForShader(filename, entryPoint))
    {
        TStringStream genSpirvCommand;
        genSpirvCommand << "dxc.exe -spirv " << GetShaderTargetByType(type) << " -E " << entryPoint << " " << filename << " -Fo " << GetSPIRVOutputFile(entryPoint);
        IOUtils::ExecuteCommand(genSpirvCommand.str());
    }
}

static Shader CreateShader(const TString& entryPoint)
{
	TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GetSPIRVOutputFile(entryPoint));

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
	static std::once_flag flag;
	std::call_once(flag, []()
	{
		GenerateSPIRVForTarget("Engine/Source/Runtime/src/Renderer/Vulkan/Shaders/FullscreenTriangle.hlsl", "fullscreenTriangleVertexShader", ShaderType::Vertex);
		GenerateSPIRVForTarget("Engine/Source/Runtime/src/Renderer/Vulkan/Shaders/FullscreenTriangle.hlsl", "fullscreenTriangleFragmentShader", ShaderType::Fragment);
	});

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
