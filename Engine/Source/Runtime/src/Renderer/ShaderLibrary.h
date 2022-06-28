#pragma once
#include "Shader.h"

namespace Gleam {

class ShaderLibrary
{
public:
    
	static void Init();
    
	static void Destroy();

	static RefCounted<Shader> CreateShader(const TString& entryPoint, ShaderStage stage);
    
	static void ClearCache();

private:

	static inline HashMap<TString, RefCounted<Shader>> mShaderCache;

};

} // namespace Gleam
