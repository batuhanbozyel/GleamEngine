#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

using namespace Gleam;

static id<MTLSamplerState> CreateMTLSamplerState(const SamplerState& samplerState)
{
    MTLSamplerDescriptor* descriptor = [MTLSamplerDescriptor new];
    descriptor.supportArgumentBuffers = YES;
    descriptor.lodMinClamp = 0.0f;
    descriptor.lodMaxClamp = Math::Infinity;
    switch (samplerState.filterMode)
    {
        case FilterMode::Point:
        {
            descriptor.minFilter = MTLSamplerMinMagFilterNearest;
            descriptor.magFilter = MTLSamplerMinMagFilterNearest;
            descriptor.mipFilter = MTLSamplerMipFilterNearest;
            break;
        }
        case FilterMode::Bilinear:
        {
            descriptor.minFilter = MTLSamplerMinMagFilterLinear;
            descriptor.magFilter = MTLSamplerMinMagFilterLinear;
            descriptor.mipFilter = MTLSamplerMipFilterNearest;
            break;
        }
        case FilterMode::Trilinear:
        {
            descriptor.minFilter = MTLSamplerMinMagFilterLinear;
            descriptor.magFilter = MTLSamplerMinMagFilterLinear;
            descriptor.mipFilter = MTLSamplerMipFilterLinear;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Filter mode is not supported!") break;
    }
    
    switch (samplerState.wrapMode)
    {
        case WrapMode::Repeat:
        {
            descriptor.sAddressMode = MTLSamplerAddressModeRepeat;
            descriptor.tAddressMode = MTLSamplerAddressModeRepeat;
            break;
        }
        case WrapMode::Clamp:
        {
            descriptor.sAddressMode = MTLSamplerAddressModeClampToEdge;
            descriptor.tAddressMode = MTLSamplerAddressModeClampToEdge;
            break;
        }
        case WrapMode::Mirror:
        {
            descriptor.sAddressMode = MTLSamplerAddressModeMirrorRepeat;
            descriptor.tAddressMode = MTLSamplerAddressModeMirrorRepeat;
            break;
        }
        case WrapMode::MirrorOnce:
        {
            descriptor.sAddressMode = MTLSamplerAddressModeMirrorClampToEdge;
            descriptor.tAddressMode = MTLSamplerAddressModeMirrorClampToEdge;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Filter mode is not supported!") break;
    }
    return [MetalDevice::GetHandle() newSamplerStateWithDescriptor:descriptor];
}

void MetalPipelineStateManager::Init()
{
    auto samplerSates = SamplerState::GetAllVariations();
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        mSamplerStates[i] = CreateMTLSamplerState(samplerSates[i]);
    }
}

void MetalPipelineStateManager::Destroy()
{
    for (uint32_t i = 0; i < mSamplerStates.size(); i++)
    {
        mSamplerStates[i] = nil;
    }
    Clear();
}

const MetalPipeline* MetalPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount)
{
	for (const auto& element : mGraphicsPipelineCache)
	{
		if (element.pipeline.descriptor == pipelineDesc &&
			element.pipeline.vertexShader == vertexShader &&
            element.pipeline.fragmentShader == fragmentShader &&
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
            
            if (found) { return &element.pipeline; }
		}
	}
    
	GraphicsPipelineCacheElement element;
    element.sampleCount = sampleCount;
	element.colorAttachments = colorAttachments;
    element.pipeline.descriptor = pipelineDesc;
    element.pipeline.vertexShader = vertexShader;
    element.pipeline.fragmentShader = fragmentShader;
    element.pipeline.topology = PrimitiveTopologyToMTLPrimitiveType(element.pipeline.descriptor.topology);
	element.pipeline.handle = CreateGraphicsPipeline(element);
    element.pipeline.depthStencil = CreateDepthStencil(pipelineDesc);
	const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
	return &cachedElement.pipeline;
}

const MetalPipeline* MetalPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const RefCounted<Shader>& vertexShader, const RefCounted<Shader>& fragmentShader, uint32_t sampleCount)
{
    for (const auto& element : mGraphicsPipelineCache)
    {
        if (element.pipeline.descriptor == pipelineDesc &&
            element.pipeline.vertexShader == vertexShader &&
            element.pipeline.fragmentShader == fragmentShader &&
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
            
            if (found) { return &element.pipeline; }
        }
    }
    
    GraphicsPipelineCacheElement element;
    element.hasDepthAttachment = true;
    element.sampleCount = sampleCount;
    element.colorAttachments = colorAttachments;
    element.depthAttachment = depthAttachment;
    element.pipeline.descriptor = pipelineDesc;
    element.pipeline.vertexShader = vertexShader;
    element.pipeline.fragmentShader = fragmentShader;
    element.pipeline.topology = PrimitiveTopologyToMTLPrimitiveType(element.pipeline.descriptor.topology);
    element.pipeline.handle = CreateGraphicsPipeline(element);
    element.pipeline.depthStencil = CreateDepthStencil(pipelineDesc);
    const auto& cachedElement = mGraphicsPipelineCache.emplace_back(element);
    return &cachedElement.pipeline;
}

id<MTLSamplerState> MetalPipelineStateManager::GetSamplerState(uint32_t index)
{
    return mSamplerStates[index];
}

id<MTLSamplerState> MetalPipelineStateManager::GetSamplerState(const SamplerState& samplerState)
{
    std::hash<SamplerState> hasher;
    return mSamplerStates[hasher(samplerState)];
}

void MetalPipelineStateManager::Clear()
{
	mGraphicsPipelineCache.clear();
}

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const GraphicsPipelineCacheElement& element)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = element.sampleCount;
    pipelineDescriptor.vertexFunction = element.pipeline.vertexShader->GetHandle();
    pipelineDescriptor.fragmentFunction = element.pipeline.fragmentShader->GetHandle();
    pipelineDescriptor.alphaToCoverageEnabled = element.pipeline.descriptor.alphaToCoverage;
    pipelineDescriptor.inputPrimitiveTopology = PrimitiveTopologyToMTLPrimitiveTopologyClass(element.pipeline.descriptor.topology);
    
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
        pipelineDescriptor.colorAttachments[i].blendingEnabled = element.pipeline.descriptor.blendState.enabled;
        pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor = BlendModeToMTLBlendFactor(element.pipeline.descriptor.blendState.sourceColorBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor = BlendModeToMTLBlendFactor(element.pipeline.descriptor.blendState.destinationColorBlendMode);
        pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor = BlendModeToMTLBlendFactor(element.pipeline.descriptor.blendState.sourceAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor = BlendModeToMTLBlendFactor(element.pipeline.descriptor.blendState.destinationAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].rgbBlendOperation = BlendOpToMTLBlendOperation(element.pipeline.descriptor.blendState.colorBlendOperation);
        pipelineDescriptor.colorAttachments[i].alphaBlendOperation = BlendOpToMTLBlendOperation(element.pipeline.descriptor.blendState.alphaBlendOperation);
        pipelineDescriptor.colorAttachments[i].writeMask = ColorWriteMaskToMTLColorWriteMask(element.pipeline.descriptor.blendState.writeMask);
    }
    
    __autoreleasing NSError* error = nil;
    id<MTLRenderPipelineState> pipeline = [MetalDevice::GetHandle() newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    GLEAM_ASSERT(pipeline, "Metal: Pipeline creation failed!");
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
