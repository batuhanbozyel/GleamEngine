#pragma once
#include "MetalUtils.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

struct MetalPipelineState
{
	id<MTLRenderPipelineState> pipeline = nil;
	bool swapchainTarget = false;
};

class MetalPipelineStateManager
{
public:

	static const MetalPipelineState& GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		PipelineStateDescriptor pipelineStateDescriptor;
		GraphicsShader program;
		MetalPipelineState pipelineState;
	};
    
	static id<MTLRenderPipelineState> CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
