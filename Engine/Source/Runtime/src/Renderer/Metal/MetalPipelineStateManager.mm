#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

using namespace Gleam;

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader)
{
	for (uint32_t i = 0; i < mGraphicsPipelineCache.size(); i++)
	{
		const auto& element = mGraphicsPipelineCache[i];
		if (element.pipelineState.descriptor == pipelineDesc &&
			element.renderPassDesc == renderPassDesc &&
			element.vertexShader == vertexShader &&
            element.fragmentShader == fragmentShader)
		{
			return element.pipelineState;
		}
	}

	GraphicsPipelineCacheElement element;
	element.vertexShader = vertexShader;
    element.fragmentShader = fragmentShader;
	element.renderPassDesc = renderPassDesc;
    element.pipelineState.descriptor = pipelineDesc;
	element.pipelineState.pipeline = CreateGraphicsPipeline(pipelineDesc, renderPassDesc, vertexShader, fragmentShader);
    element.pipelineState.depthStencil = CreateDepthStencil(pipelineDesc);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipelineState;
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const RenderPassDescriptor& renderPassDesc, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = fragmentShader->GetHandle();
    
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
