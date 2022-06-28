#pragma once
#include "MetalUtils.h"
#include "Renderer/Shader.h"
#include "Renderer/RenderPassDescriptor.h"
#include "Renderer/PipelineStateDescriptor.h"

namespace Gleam {

struct MetalPipelineState
{
    MTLRenderPassDescriptor* renderPass = nil;
	id<MTLRenderPipelineState> pipeline;
	bool swapchainTarget = false;
};

class MetalPipelineStateManager
{
public:

	static const MetalPipelineState& GetGraphicsPipelineState(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		RenderPassDescriptor renderPassDescriptor;
		PipelineStateDescriptor pipelineStateDescriptor;
		GraphicsShader program;
		MetalPipelineState pipelineState;
	};


	static MTLRenderPassDescriptor* CreateRenderPass(const RenderPassDescriptor& renderPassDesc);

	static id<MTLRenderPipelineState> CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
