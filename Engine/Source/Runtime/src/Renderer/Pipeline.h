#pragma once
#include "PipelineStateDescriptor.h"

namespace Gleam {

class RenderPass;
class GraphicsShader;

class GraphicsPipeline
{
public:

	GraphicsPipeline(const RenderPass& renderPass, const PipelineStateDescriptor& pipelineStateDescriptor, const GraphicsShader& shader);
	~GraphicsPipeline();

	NativeGraphicsHandle GetPipeline() const
	{
		return mPipeline;
	}

	NativeGraphicsHandle GetLayout() const
	{
		return mLayout;
	}

private:

	NativeGraphicsHandle mPipeline;
	NativeGraphicsHandle mLayout;


};

} // namespace Gleam