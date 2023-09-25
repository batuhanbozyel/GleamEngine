#pragma once
#ifdef USE_METAL_RENDERER
#include "MetalUtils.h"
#include "Renderer/Shader.h"
#include "Renderer/SamplerState.h"

namespace Gleam {

struct MetalPipeline
{
	id<MTLRenderPipelineState> handle = nil;
	id<MTLDepthStencilState> depthStencil = nil;
	MTLPrimitiveType topology = MTLPrimitiveTypeTriangle;
    PipelineStateDescriptor descriptor;
};

struct MetalGraphicsPipeline : public MetalPipeline
{
    RefCounted<Shader> vertexShader;
    RefCounted<Shader> fragmentShader;
};

class MetalPipelineStateManager
{
public:
    
    static void Init();
    
    static void Destroy();

	static const MetalPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& attachmentDescriptors, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount);
    
    static const MetalPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& attachmentDescriptors, const TextureDescriptor& depthAttachment, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount);
    
    static id<MTLSamplerState> GetSamplerState(uint32_t index);
    
    static id<MTLSamplerState> GetSamplerState(const SamplerState& samplerState);

	static void Clear();

private:

	struct GraphicsPipelineCacheElement
	{
		MetalGraphicsPipeline pipeline;
		TArray<TextureDescriptor> colorAttachments;
        TextureDescriptor depthAttachment;
        bool hasDepthAttachment = false;
        uint32_t sampleCount = 1;
	};
    
    static id<MTLRenderPipelineState> CreateGraphicsPipeline(const GraphicsPipelineCacheElement& element);
    
    static id<MTLDepthStencilState> CreateDepthStencil(const PipelineStateDescriptor& pipelineDesc);

    static inline TArray<id<MTLSamplerState>, 12> mSamplerStates;
    
	static inline TArray<GraphicsPipelineCacheElement> mGraphicsPipelineCache;

};

} // namespace Gleam
#endif
