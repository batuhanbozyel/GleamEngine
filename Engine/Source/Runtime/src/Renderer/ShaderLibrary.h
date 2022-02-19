#pragma once
#include "Shader.h"

namespace Gleam {

enum BuiltinGraphicsShader
{
	FullscreenTriangleShader,
	ForwardPassShader,
	BuiltinGrahpicsShaderCOUNT //
};

enum BuiltinComputeShader
{
	BuiltinComputeShaderCOUNT //
};

class ShaderLibrary
{
public:
    
	static void Init();
    
	static void Destroy();

	static const Ref<GraphicsShader>& GetBuiltinGraphicsShader(BuiltinGraphicsShader id);

	static const Ref<ComputeShader>& GetBuiltinComputeShader(BuiltinComputeShader id);
    
	static Ref<GraphicsShader> CreateGraphicsShader(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint);

	static Ref<ComputeShader> CreateComputeShader(const TString& filename, const TString& entryPoint);
    
	static void ClearCache();

private:

	static inline HashMap<TString, Ref<GraphicsShader>> mGraphicsShaderCache;
	static inline HashMap<TString, Ref<ComputeShader>> mComputeShaderCache;

};

} // namespace Gleam
