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
	Ref<Shader> vertexShader;
	Ref<Shader> fragmentShader;
    
    bool operator==(const GraphicsShader&) const = default;
};

using ComputeShader = Ref<Shader>;


} // namespace Gleam
