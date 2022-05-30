#pragma once

namespace Gleam {

enum class ShaderStage
{
	Vertex,
	Fragment,
	Compute
};

class Shader : public GraphicsObject
{
public:

	Shader(const TString& entryPoint);
	~Shader();

	const TString& GetEntryPoint() const
	{
		return mEntryPoint;
	}

private:

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
