#include "gpch.h"

#ifdef USE_VULKAN_RENDERER
#include "Renderer/ShaderLibrary.h"
#include "VulkanUtils.h"

using namespace Gleam;

static HashMap<TString, VkShaderModule> mShaderCache;

static VkShaderModule CreateShader(VkDevice device, TArray<char> source)
{
	VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	createInfo.codeSize = source.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(source.data());

	VkShaderModule shader;
	VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shader));

	return shader;
}

static Shader CreateOrGetCachedShader(VkDevice device, TArray<char> source, const TString& entryPoint)
{
	Shader shader;
	shader.entryPoint = entryPoint;

	auto shaderCacheIt = mShaderCache.find(entryPoint);
	if (shaderCacheIt != mShaderCache.end())
	{
		shader.handle = shaderCacheIt->second;
		return shader;
	}

	VkShaderModule handle = CreateShader(device, source);
	mShaderCache.insert(mShaderCache.end(), { entryPoint, handle });

	shader.handle = handle;
	return shader;
}

ShaderProgram ShaderLibrary::CreateShaderProgram(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
	TString vertexShaderFile = filename + ".vert.spv";
	TString fragmentShaderFile = filename + ".frag.spv";

	TArray<char> vertexShaderCode = FileUtils::ReadBinaryFile(vertexShaderFile);
	TArray<char> fragmentShaderCode = FileUtils::ReadBinaryFile(fragmentShaderFile);

	ShaderProgram program;
	program.vertexShader = CreateOrGetCachedShader(VulkanDevice, vertexShaderCode, vertexEntryPoint);
	program.fragmentShader = CreateOrGetCachedShader(VulkanDevice, fragmentShaderCode, fragmentEntryPoint);
	return program;
}

Shader ShaderLibrary::CreateComputeShader(const TString& filename, const TString& entryPoint)
{
	TString computeShaderFile = filename + ".comp.spv";
	TArray<char> computeShaderCode = FileUtils::ReadBinaryFile(computeShaderFile);
	return CreateOrGetCachedShader(VulkanDevice, computeShaderCode, entryPoint);
}

void ShaderLibrary::ClearCache()
{
	for (auto[_, shader] : mShaderCache)
	{
		vkDestroyShaderModule(VulkanDevice, shader, nullptr);
	}
	mShaderCache.clear();
}

#endif