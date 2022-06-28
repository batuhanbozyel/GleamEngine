#include "gpch.h"
#include "ShaderLibrary.h"

using namespace Gleam;

void ShaderLibrary::Destroy()
{
	ClearCache();
}

void ShaderLibrary::ClearCache()
{
	mShaderCache.clear();
}

RefCounted<Shader> ShaderLibrary::CreateShader(const TString& entryPoint, ShaderStage stage)
{
	const auto& shaderCacheIt = mShaderCache.find(entryPoint);
	if (shaderCacheIt != mShaderCache.end())
	{
		return shaderCacheIt->second;
	}
	else
	{
		return CreateRef<Shader>(entryPoint, stage);
	}
}
