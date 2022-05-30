#include "gpch.h"
#include "ShaderLibrary.h"

using namespace Gleam;

/************************************************************************/
/*    Init                                                              */
/************************************************************************/
void ShaderLibrary::Init()
{
	
}
/************************************************************************/
/*    Destroy                                                           */
/************************************************************************/
void ShaderLibrary::Destroy()
{
	ClearCache();
}
/************************************************************************/
/*    ClearCache                                                        */
/************************************************************************/
void ShaderLibrary::ClearCache()
{
	mShaderCache.clear();
}
/************************************************************************/
/*    CreateGraphicsShader                                              */
/************************************************************************/
GraphicsShader ShaderLibrary::CreateGraphicsShader(const TString& vertexEntryPoint, const TString& fragmentEntryPoint)
{
	GraphicsShader shader;
	shader.vertexShader = CreateOrGetCachedShader(vertexEntryPoint);
	shader.fragmentShader = CreateOrGetCachedShader(fragmentEntryPoint);
	return shader;
}
/************************************************************************/
/*    CreateComputeShader                                               */
/************************************************************************/
ComputeShader ShaderLibrary::CreateComputeShader(const TString& entryPoint)
{
	return CreateOrGetCachedShader(entryPoint);
}
/************************************************************************/
/*    CreateOrGetCachedShader                                           */
/************************************************************************/
RefCounted<Shader> ShaderLibrary::CreateOrGetCachedShader(const TString& entryPoint)
{
	const auto& shaderCacheIt = mShaderCache.find(entryPoint);
	if (shaderCacheIt != mShaderCache.end())
	{
		return shaderCacheIt->second;
	}
	else
	{
		return CreateRef<Shader>(entryPoint);
	}
}
