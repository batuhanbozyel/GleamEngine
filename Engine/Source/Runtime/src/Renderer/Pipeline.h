#pragma once
#include "PipelineStateDescriptor.h"

namespace Gleam {

class GraphicsShader;

class GraphicsPipeline
{
public:

	GraphicsPipeline(const PipelineStateDescriptor& pipelineStateDescriptor, const Ref<GraphicsShader>& shader);
	~GraphicsPipeline();

private:

	NativeGraphicsHandle mPipeline;
	NativeGraphicsHandle mLayout;


};

} // namespace Gleam