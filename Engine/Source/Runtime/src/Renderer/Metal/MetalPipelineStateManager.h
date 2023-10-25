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
};

struct MetalGraphicsPipeline : public MetalPipeline
{
    Shader vertexShader;
    Shader fragmentShader;
};

class MetalPipelineStateManager
{
public:
    
    static void Init(MetalDevice* device);

    static void Destroy();

    static const MetalPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);

    static const MetalPipeline* GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);

    static id<MTLSamplerState> GetSamplerState(uint32_t index);

    static id<MTLSamplerState> GetSamplerState(const SamplerState& samplerState);

    static void Clear();

private:
    
    static id<MTLRenderPipelineState> CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount);

    static id<MTLDepthStencilState> CreateDepthStencil(const PipelineStateDescriptor& pipelineDesc);

    static inline TArray<id<MTLSamplerState>, 12> mSamplerStates;

    static inline HashMap<size_t, Scope<MetalGraphicsPipeline>> mGraphicsPipelineCache;
    
    static inline MetalDevice* mDevice = nullptr;

};

} // namespace Gleam
#endif
