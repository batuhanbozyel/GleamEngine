#pragma once
#include "Shader.h"

namespace Gleam {

class ShaderLibrary
{
public:
    
	static void Init();
    
	static void Destroy();
    
	static GraphicsShader CreateGraphicsShader(const TString& vertexEntryPoint, const TString& fragmentEntryPoint);

	static ComputeShader CreateComputeShader(const TString& entryPoint);
    
	static void ClearCache();

private:

	static Ref<Shader> CreateOrGetCachedShader(const TString& entryPoint);

	static inline HashMap<TString, Ref<Shader>> mShaderCache;

};

} // namespace Gleam
