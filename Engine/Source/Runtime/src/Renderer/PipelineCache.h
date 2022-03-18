#pragma once
#include "Pipeline.h"

namespace Gleam {

class PipelineCache
{
public:

	static void Init();

	static void Destroy();

	static Ref<GraphicsPipeline> CreateGraphicsPipeline(const RenderPass& renderPass, const PipelineStateDescriptor& pipelineStateDescriptor, const GraphicsShader& shader);

	static void Flush();

private:

	//static inline Map<PipelineStateDescriptor, NativeGraphicsHandle> mGraphicsPipelineCache;

};

} // namespace Gleam