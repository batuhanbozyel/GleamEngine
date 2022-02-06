#pragma once

namespace Gleam {

enum class ShaderType
{
	Vertex,
	Fragment,
	Compute
};

struct Shader
{
	TString entryPoint;
	NativeGraphicsHandle function;
};

class GraphicsShader
{
public:

	GraphicsShader(const TString& vertexEntryPoint, const TString& fragmentEntryPoint);
	~GraphicsShader();

	const Shader& GetVertexShader() const
	{
		return mVertexShader;
	}

	const Shader& GetFragmentShader() const
	{
		return mFragmentShader;
	}

private:

	Shader mVertexShader;
	Shader mFragmentShader;

};

class ComputeShader
{
public:

	ComputeShader(const TString& entryPoint);
	~ComputeShader();

	const Shader& GetShader() const
	{
		return mShader;
	}

private:

	Shader mShader;

};


} // namespace Gleam