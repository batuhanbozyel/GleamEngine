#pragma once
#include "Shader.h"
#include "PipelineStateDescriptor.h"

namespace Gleam {

class Material
{
public:

	const TArray<RefCounted<Shader>> GetProgram() const
	{
		return mProgram;
	}

	const PipelineStateDescriptor& GetPipelineState() const
	{
		return mPipelineState;
	}

private:

	TArray<RefCounted<Shader>> mProgram{};
	PipelineStateDescriptor mPipelineState{};

};

} // namespace Gleam
