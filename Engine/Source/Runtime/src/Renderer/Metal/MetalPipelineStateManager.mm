#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

using namespace Gleam;

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const GraphicsShader& program)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipelineState.descriptor == pipelineDesc &&
			element.renderPassDesc = renderPassDesc &&
			element.program == program)
		{
			return element.pipelineState;
		}
	}

	GraphicsPipelineCacheElement element;
	element.program = program;
	element.renderPassDesc = renderPassDesc;
    element.pipelineState.descriptor = pipelineDesc;
	element.pipelineState.pipeline = CreateGraphicsPipeline(pipelineDesc, renderPassDesc, program);
    element.pipelineState.depthStencil = CreateDepthStencil(pipelineDesc);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipelineState;
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const GraphicsShader& program)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = program.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = program.fragmentShader->GetHandle();
    
    if (renderPassDesc.depthAttachmentIndex >= 0)
    {
        MTLPixelFormat format = TextureFormatToMTLPixelFormat(renderPassDesc.attachments[renderPassDesc.depthAttachmentIndex].format);
        pipelineDescriptor.depthAttachmentPixelFormat = format;
        pipelineDescriptor.stencilAttachmentPixelFormat = format;
    }
    
    for (uint32_t i = 0; i < renderPassDesc.attachments.size(); i++)
	{
        uint32_t attachmentIndexOffset = static_cast<int>(renderPassDesc.depthAttachmentIndex <= i);
        uint32_t colorAttachmentIndex = i + attachmentIndexOffset;
        
        const auto& attachmentDesc = renderPassDesc.attachments[colorAttachmentIndex];
		pipelineDescriptor.colorAttachments[i].pixelFormat = TextureFormatToMTLPixelFormat(attachmentDesc.format);
        /**
                TODO:
         */
	}
    
    NSError* error;
    id<MTLRenderPipelineState> pipeline = [MetalDevice::GetHandle() newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    return pipeline;
}

id<MTLDepthStencilState> MetalPipelineStateManager::CreateDepthStencil(const PipelineStateDescriptor& pipelineDesc)
{
    MTLDepthStencilDescriptor* depthStencilDesc = [MTLDepthStencilDescriptor new];
    depthStencilDesc.depthWriteEnabled = pipelineDesc.depthState.writeEnabled;
    depthStencilDesc.depthCompareFunction = CompareFunctionToMTLCompareFunction(pipelineDesc.depthState.compareFunction);
    /*
     TODO: Setup stencil state
     */
    id<MTLDepthStencilState> depthStencilState = [MetalDevice::GetHandle() newDepthStencilStateWithDescriptor:depthStencilDesc];
    return depthStencilState;
}
#endif
