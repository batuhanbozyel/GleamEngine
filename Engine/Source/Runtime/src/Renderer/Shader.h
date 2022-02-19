#pragma once

namespace Gleam {

enum class ShaderType
{
	Vertex,
	Fragment,
	Compute
};

class Shader
{
public:

	Shader(const TString& entryPoint);
	~Shader();

	const TString& GetEntryPoint() const
	{
		return mEntryPoint;
	}

	NativeGraphicsHandle GetFunction() const
	{
		return mFunction;
	}

private:

	TString mEntryPoint;
	NativeGraphicsHandle mFunction;

};

struct GraphicsShader
{
	Ref<Shader> vertexShader;
	Ref<Shader> fragmentShader;
};

using ComputeShader = Ref<Shader>;


} // namespace Gleam