#pragma once

namespace Gleam {

struct Shader
{
	handle_t handle;
	TString entryPoint;
};

struct ShaderProgram
{
	Shader vertexShader;
	Shader fragmentShader;
};

class ShaderLibrary
{
public:
    
    static void Init();
    
    static void Destroy();
    
    static ShaderProgram CreatePrecompiledShaderProgram(const TString& vertexEntryPoint, const TString& fragmentEntryPoint);

    static Shader CreatePrecompiledComputeShader(const TString& entryPoint);
    
	static ShaderProgram CreateShaderProgram(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint);

	static Shader CreateComputeShader(const TString& filename, const TString& entryPoint);
    
	static void ClearCache();

};

} // namespace Gleam
