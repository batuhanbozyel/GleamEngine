#pragma once

namespace Gleam {

struct Shader
{
	void* handle;
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

	static ShaderProgram CreateShaderProgram(const TString& filename, const TString& vertexEntryPoint, const TString& fragmentEntryPoint);

	static Shader CreateComputeShader(const TString& filename, const TString& entryPoint);

	static void ClearCache();

};

} // namespace Gleam