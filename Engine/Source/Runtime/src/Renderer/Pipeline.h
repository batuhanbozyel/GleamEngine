#pragma once
#include "PipelineStateDescriptor.h"

namespace Gleam {

class RenderPass;
class GraphicsShader;

class GraphicsPipeline : public GraphicsObject
{
public:

	GraphicsPipeline(const RenderPass& renderPass, const PipelineStateDescriptor& pipelineStateDescriptor, const GraphicsShader& shader);
	~GraphicsPipeline();

	NativeGraphicsHandle GetLayout() const
	{
		return mLayout;
	}

private:

	NativeGraphicsHandle mLayout;


};

} // namespace Gleam