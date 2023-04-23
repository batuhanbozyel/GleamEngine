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

	static const MetalPipelineState& GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& attachmentDescriptors, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount);
    
    static const MetalPipelineState& GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& attachmentDescriptors, const TextureDescriptor& depthAttachment, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		RefCounted<Shader> vertexShader;
		RefCounted<Shader> fragmentShader;
		MetalPipelineState pipelineState;
		TArray<TextureDescriptor> colorAttachments;
        TextureDescriptor depthAttachment;
        bool hasDepthAttachment = false;
        uint32_t sampleCount = 1;
	};
    
    static id<MTLRenderPipelineState> CreateGraphicsPipeline(const GraphicsPipelineCacheElement& element);
    
    static id<MTLDepthStencilState> CreateDepthStencil(const PipelineStateDescriptor& pipelineDesc);

	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif
