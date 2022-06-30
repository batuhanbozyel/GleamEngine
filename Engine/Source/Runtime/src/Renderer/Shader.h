#pragma once

namespace Gleam {

enum class ShaderStage
{
	Vertex,
	Fragment,
	Compute
};

class Shader final : public GraphicsObject
{
public:

	Shader(const TString& entryPoint, ShaderStage stage);
	~Shader();

	ShaderStage GetStage() const
	{
		return mStage;
	}

	const TString& GetEntryPoint() const
	{
		return mEntryPoint;
	}

	struct Reflection;
	Scope<Reflection> reflection;

private:

	ShaderStage mStage;
	TString mEntryPoint;

};

struct GraphicsShader
{
	RefCounted<Shader> vertexShader;
	RefCounted<Shader> fragmentShader;
    
    bool operator==(const GraphicsShader&) const = default;
};

using ComputeShader = RefCounted<Shader>;


} // namespace Gleam
