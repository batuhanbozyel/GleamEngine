#pragma once
#ifdef USE_METAL_RENDERER
#include "MetalUtils.h"
#include "Renderer/Shader.h"

namespace Gleam {

struct MetalPipelineState
{
    PipelineStateDescriptor descriptor;
	id<MTLRenderPipelineState> pipeline = nil;
    id<MTLDepthStencilState> depthStencil = nil;
};

class MetalPipelineStateManager
{
public:

	static const MetalPipelineState& GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const GraphicsShader& program);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		GraphicsShader program;
		MetalPipelineState pipelineState;
		RenderPassDescriptor renderPassDesc;
		TArray<TextureFormat> attachmentFormats;
	};
    
	static id<MTLRenderPipelineState> CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const GraphicsShader& program);
    
    static id<MTLDepthStencilState> CreateDepthStencil(const PipelineStateDescriptor& pipelineDesc);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif
