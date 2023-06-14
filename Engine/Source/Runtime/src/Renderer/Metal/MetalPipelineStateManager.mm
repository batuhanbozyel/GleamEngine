#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

using namespace Gleam;

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount)
{
	for (const auto& element : mGraphicsPipelineCache)
	{
		if (element.pipelineState.descriptor == pipelineDesc &&
			element.vertexShader == vertexShader &&
            element.fragmentShader == fragmentShader &&
            element.colorAttachments.size() == colorAttachments.size() &&
            element.sampleCount == sampleCount &&
            !element.hasDepthAttachment)
		{
            bool found = true;
            for (uint32_t i = 0; i < colorAttachments.size(); i++)
            {
                if (element.colorAttachments[i] != colorAttachments[i])
                {
                    found = false;
                    break;
                }
            }
            
            if (found) { return element.pipelineState; }
		}
	}
    
	GraphicsPipelineCacheElement element;
    element.sampleCount = sampleCount;
	element.vertexShader = vertexShader;
    element.fragmentShader = fragmentShader;
	element.colorAttachments = colorAttachments;
    element.pipelineState.descriptor = pipelineDesc;
	element.pipelineState.pipeline = CreateGraphicsPipeline(element);
    element.pipelineState.depthStencil = CreateDepthStencil(pipelineDesc);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return cachedElement.pipelineState;
}

const MetalPipelineState& MetalPipelineStateManager::GetGraphicsPipelineState(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount)
{
    for (const auto& element : mGraphicsPipelineCache)
    {
        if (element.pipelineState.descriptor == pipelineDesc &&
            element.vertexShader == vertexShader &&
            element.fragmentShader == fragmentShader &&
            element.colorAttachments.size() == colorAttachments.size() &&
            element.depthAttachment == depthAttachment &&
            element.sampleCount == sampleCount &&
            element.hasDepthAttachment)
        {
            bool found = true;
            for (uint32_t i = 0; i < colorAttachments.size(); i++)
            {
                if (element.colorAttachments[i] != colorAttachments[i])
                {
                    found = false;
                    break;
                }
            }
            
            if (found) { return element.pipelineState; }
        }
    }
    
    GraphicsPipelineCacheElement element;
    element.hasDepthAttachment = true;
    element.sampleCount = sampleCount;
    element.vertexShader = vertexShader;
    element.fragmentShader = fragmentShader;
    element.colorAttachments = colorAttachments;
    element.depthAttachment = depthAttachment;
    element.pipelineState.descriptor = pipelineDesc;
    element.pipelineState.pipeline = CreateGraphicsPipeline(element);
    element.pipelineState.depthStencil = CreateDepthStencil(pipelineDesc);
    const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
    return cachedElement.pipelineState;
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const GraphicsPipelineCacheElement& element)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.vertexFunction = element.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = element.fragmentShader->GetHandle();
    pipelineDescriptor.rasterSampleCount = element.sampleCount;
    pipelineDescriptor.alphaToCoverageEnabled = element.pipelineState.descriptor.alphaToCoverage;
    pipelineDescriptor.inputPrimitiveTopology = PrimitiveToplogyToMTLPrimitiveTopologyClass(element.pipelineState.descriptor.topology);
    
    if (element.hasDepthAttachment)
    {
        MTLPixelFormat format = TextureFormatToMTLPixelFormat(element.depthAttachment.format);
        
        if (Utils::IsDepthFormat(element.depthAttachment.format)) { pipelineDescriptor.depthAttachmentPixelFormat = format; }
        if (Utils::IsStencilFormat(element.depthAttachment.format)) { pipelineDescriptor.stencilAttachmentPixelFormat = format; }
    }
    
    for (uint32_t i = 0; i < element.colorAttachments.size(); i++)
    {
        const auto& attachmentDesc = element.colorAttachments[i];
        pipelineDescriptor.colorAttachments[i].pixelFormat = TextureFormatToMTLPixelFormat(attachmentDesc.format);
        pipelineDescriptor.colorAttachments[i].blendingEnabled = element.pipelineState.descriptor.blendState.enabled;
        pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor = BlendModeToMTLBlendFactor(element.pipelineState.descriptor.blendState.sourceColorBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor = BlendModeToMTLBlendFactor(element.pipelineState.descriptor.blendState.destinationColorBlendMode);
        pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor = BlendModeToMTLBlendFactor(element.pipelineState.descriptor.blendState.sourceAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor = BlendModeToMTLBlendFactor(element.pipelineState.descriptor.blendState.destinationAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].rgbBlendOperation = BlendOpToMTLBlendOperation(element.pipelineState.descriptor.blendState.colorBlendOperation);
        pipelineDescriptor.colorAttachments[i].alphaBlendOperation = BlendOpToMTLBlendOperation(element.pipelineState.descriptor.blendState.alphaBlendOperation);
        pipelineDescriptor.colorAttachments[i].writeMask = ColorWriteMaskToMTLColorWriteMask(element.pipelineState.descriptor.blendState.writeMask);
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
    
    if (pipelineDesc.stencilState.enabled)
    {
        MTLStencilDescriptor* stencilDesc = [MTLStencilDescriptor new];
        stencilDesc.readMask = pipelineDesc.stencilState.readMask;
        stencilDesc.writeMask = pipelineDesc.stencilState.writeMask;
        stencilDesc.stencilCompareFunction = CompareFunctionToMTLCompareFunction(pipelineDesc.stencilState.compareFunction);
        stencilDesc.depthFailureOperation = StencilOpToMTLStencilOperation(pipelineDesc.stencilState.depthFailOperation);
        stencilDesc.stencilFailureOperation = StencilOpToMTLStencilOperation(pipelineDesc.stencilState.depthFailOperation);
        stencilDesc.depthStencilPassOperation = StencilOpToMTLStencilOperation(pipelineDesc.stencilState.passOperation);
        
        depthStencilDesc.backFaceStencil = stencilDesc;
        depthStencilDesc.frontFaceStencil = stencilDesc;
    }
    
    id<MTLDepthStencilState> depthStencilState = [MetalDevice::GetHandle() newDepthStencilStateWithDescriptor:depthStencilDesc];
    return depthStencilState;
}
#endif
