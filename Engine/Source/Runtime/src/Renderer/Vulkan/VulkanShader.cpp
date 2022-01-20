#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/ShaderLibrary.h"
#include "VulkanUtils.h"

using namespace Gleam;

static HashMap<TString, VkShaderModule> mShaderCache;

static VkShaderModule CreateShader(const TArray<uint8_t>& source)
{
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = source.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(source.data());

	VkShaderModule shader;
	VK_CHECK(vkCreateShaderModule(VulkanDevice, &createInfo, nullptr, &shader));

	return shader;
}

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
    // TODO: Make SPIR-V generation as part of development so that distributed code won't even check this
    if (ShouldGenerateSPIRVForShader(filename, entryPoint))
    {
        TStringStream genSpirvCommand;
        genSpirvCommand << "dxc.exe -spirv " << GetShaderTargetByType(type) << " -E " << entryPoint << " " << filename << " - Fo " << GetSPIRVOutputFile(entryPoint);
        IOUtils::ExecuteCommand(genSpirvCommand.str());
    }
}

/************************************************************************/
/*    Init                                                              */
/************************************************************************/
void ShaderLibrary::Init()
{
    auto GetShaderFileByName = [](const TString& shaderFile)
    {
        return "Engine/Source/Runtime/src/Renderer/Vulkan/Shaders/" + shaderFile + ".hlsl";
    };
    
    GenerateSPIRVForTarget(GetShaderFileByName("FullscreenTriangle"), "fullscreenTriangleVertexShader", ShaderType::Vertex);
    GenerateSPIRVForTarget(GetShaderFileByName("FullscreenTriangle"), "fullscreenTriangleFragmentShader", ShaderType::Fragment);
}
/************************************************************************/
/*    Destroy                                                           */
/************************************************************************/
void ShaderLibrary::Destroy()
{
    ClearCache();
}
/************************************************************************/
/*    CreateOrGetCachedShaderFromLibrary                                */
/************************************************************************/
static Shader CreateOrGetCachedShader(const TString& entryPoint)
{
    Shader shader;
    shader.entryPoint = entryPoint;
    
    auto shaderCacheIt = mShaderCache.find(entryPoint);
    if (shaderCacheIt != mShaderCache.end())
    {
        shader.handle = shaderCacheIt->second;
        return shader;
    }
    
    TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GetSPIRVOutputFile(entryPoint));
    VkShaderModule handle = CreateShader(shaderCode);
    mShaderCache.insert(mShaderCache.end(), { entryPoint, handle });
    
    shader.handle = handle;
    return shader;
}
/************************************************************************/
/*    CreatePrecompiledShaderProgram                                    */
/************************************************************************/
ShaderProgram ShaderLibrary::CreatePrecompiledShaderProgram(const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
    ShaderProgram program;
    program.vertexShader = CreateOrGetCachedShader(vertexEntryPoint);
    program.fragmentShader = CreateOrGetCachedShader(fragmentEntryPoint);
    return program;
}
/************************************************************************/
/*    CreatePrecompiledComputeShader                                    */
/************************************************************************/
Shader ShaderLibrary::CreatePrecompiledComputeShader(const TString& entryPoint)
{
    return CreateOrGetCachedShader(entryPoint);
}
/************************************************************************/
/*    CreateShaderProgram                                               */
/************************************************************************/
ShaderProgram ShaderLibrary::CreateShaderProgram(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
    GenerateSPIRVForTarget(filename, vertexEntryPoint, ShaderType::Vertex);
    GenerateSPIRVForTarget(filename, fragmentEntryPoint, ShaderType::Fragment);
    
    return CreatePrecompiledShaderProgram(vertexEntryPoint, fragmentEntryPoint);
}
/************************************************************************/
/*    CreateComputeShader                                               */
/************************************************************************/
Shader ShaderLibrary::CreateComputeShader(const TString& filename, const TString& entryPoint)
{
    GenerateSPIRVForTarget(filename, entryPoint, ShaderType::Compute);
    
    return CreatePrecompiledComputeShader(entryPoint);
}
/************************************************************************/
/*    ClearCache                                                        */
/************************************************************************/
void ShaderLibrary::ClearCache()
{
	for (auto[_, shader] : mShaderCache)
	{
		vkDestroyShaderModule(VulkanDevice, shader, nullptr);
	}
	mShaderCache.clear();
}

#endif
