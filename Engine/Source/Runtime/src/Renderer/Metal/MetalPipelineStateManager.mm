#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

#include "Renderer/Renderer.h"

using namespace Gleam;

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const RenderPassDescriptor& renderPassDesc, const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.renderPassDescriptor == renderPassDesc
			&& element.pipelineStateDescriptor == pipelineDesc
			&& element.program == program)
		{
			return element.pipelineState;
		}
	}

	GraphicsPipelineCacheElement element;
	element.renderPassDescriptor = renderPassDesc;
	element.pipelineStateDescriptor = pipelineDesc;
	element.program = program;
	element.pipelineState.renderPass = CreateRenderPass(renderPassDesc);
	element.pipelineState.pipeline = CreateGraphicsPipeline(pipelineDesc, program);
	element.pipelineState.swapchainTarget = renderPassDesc.swapchainTarget;
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipelineState;
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

MTLRenderPassDescriptor* MetalPipelineStateManager::CreateRenderPass(const RenderPassDescriptor& renderPassDesc)
{
    MTLRenderPassDescriptor* renderPass = [MTLRenderPassDescriptor renderPassDescriptor];
    
    /**
            TODO:
     */
    
    return renderPass;
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const GraphicsShader& program)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = program.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = program.fragmentShader->GetHandle();
    pipelineDescriptor.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
    
    /**
            TODO:
     */
    
    NSError* error;
    id<MTLRenderPipelineState> pipeline = [MetalDevice newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    return pipeline;
}
#endif
