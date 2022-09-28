#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

#include "Renderer/Renderer.h"

using namespace Gleam;

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipelineState.descriptor == pipelineDesc && element.program == program)
		{
			return element.pipelineState;
		}
	}

	GraphicsPipelineCacheElement element;
	element.program = program;
    element.pipelineState.descriptor = pipelineDesc;
	element.pipelineState.pipeline = CreateGraphicsPipeline(pipelineDesc, program);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipelineState;
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = program.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = program.fragmentShader->GetHandle();
    pipelineDescriptor.colorAttachments[0].pixelFormat = TextureFormatToMTLPixelFormat(RendererContext::GetSwapchain()->GetFormat());
    
    /**
            TODO:
     */
    
    NSError* error;
    id<MTLRenderPipelineState> pipeline = [MetalDevice newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    return pipeline;
}
#endif
