#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalPipelineStateManager.h"

using namespace Gleam;

static size_t PipelineHasher(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    size_t hash = 0;
    hash_combine(hash, pipelineDesc);
    hash_combine(hash, vertexShader);
    hash_combine(hash, fragmentShader);
    for (const auto& colorAttachment : colorAttachments)
    {
        hash_combine(hash, colorAttachment.format);
    }
    hash_combine(hash, depthAttachment.format);
    hash_combine(hash, sampleCount);
    return hash;
}

static id<MTLSamplerState> CreateMTLSamplerState(MetalDevice* device, const SamplerState& samplerState)
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
    return [device->GetHandle() newSamplerStateWithDescriptor:descriptor];
}

void MetalPipelineStateManager::Init(MetalDevice* device)
{
    mDevice = device;
    auto samplerSates = SamplerState::GetAllVariations();
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        mSamplerStates[i] = CreateMTLSamplerState(device, samplerSates[i]);
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

const MetalPipeline* MetalPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    return GetGraphicsPipeline(pipelineDesc, colorAttachments, TextureDescriptor(), vertexShader, fragmentShader, sampleCount);
}

const MetalPipeline* MetalPipelineStateManager::GetGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    auto key = PipelineHasher(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, sampleCount);
    auto it = mGraphicsPipelineCache.find(key);
    if (it != mGraphicsPipelineCache.end())
    {
        return it->second.get();
    }
    
    auto pipeline = new MetalGraphicsPipeline;
    pipeline->topology = PrimitiveTopologyToMTLPrimitiveType(pipelineDesc.topology);
    pipeline->handle = CreateGraphicsPipeline(pipelineDesc, colorAttachments, depthAttachment, vertexShader, fragmentShader, sampleCount);
    pipeline->depthStencil = Utils::IsDepthFormat(depthAttachment.format) ? CreateDepthStencil(pipelineDesc) : nil;
    pipeline->vertexShader = vertexShader;
    pipeline->fragmentShader = fragmentShader;
    mGraphicsPipelineCache.insert(mGraphicsPipelineCache.end(), {key, Scope<MetalGraphicsPipeline>(pipeline)});
    return pipeline;
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

id<MTLRenderPipelineState> MetalPipelineStateManager::CreateGraphicsPipeline(const PipelineStateDescriptor& pipelineDesc, const TArray<TextureDescriptor>& colorAttachments, const TextureDescriptor& depthAttachment, const Shader& vertexShader, const Shader& fragmentShader, uint32_t sampleCount)
{
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = sampleCount;
    pipelineDescriptor.vertexFunction = vertexShader.GetHandle();
    pipelineDescriptor.fragmentFunction = fragmentShader.GetHandle();
    pipelineDescriptor.alphaToCoverageEnabled = pipelineDesc.alphaToCoverage;
    pipelineDescriptor.inputPrimitiveTopology = PrimitiveTopologyToMTLPrimitiveTopologyClass(pipelineDesc.topology);
    
    if (Utils::IsDepthFormat(depthAttachment.format))
    {
        MTLPixelFormat format = TextureFormatToMTLPixelFormat(depthAttachment.format);
        
        if (Utils::IsDepthFormat(depthAttachment.format)) { pipelineDescriptor.depthAttachmentPixelFormat = format; }
        if (Utils::IsDepthStencilFormat(depthAttachment.format)) { pipelineDescriptor.stencilAttachmentPixelFormat = format; }
    }
    
    for (uint32_t i = 0; i < colorAttachments.size(); i++)
    {
        const auto& attachmentDesc = colorAttachments[i];
        pipelineDescriptor.colorAttachments[i].pixelFormat = TextureFormatToMTLPixelFormat(attachmentDesc.format);
        pipelineDescriptor.colorAttachments[i].blendingEnabled = pipelineDesc.blendState.enabled;
        pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.sourceColorBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.destinationColorBlendMode);
        pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.sourceAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.destinationAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].rgbBlendOperation = BlendOpToMTLBlendOperation(pipelineDesc.blendState.colorBlendOperation);
        pipelineDescriptor.colorAttachments[i].alphaBlendOperation = BlendOpToMTLBlendOperation(pipelineDesc.blendState.alphaBlendOperation);
        pipelineDescriptor.colorAttachments[i].writeMask = ColorWriteMaskToMTLColorWriteMask(pipelineDesc.blendState.writeMask);
    }
    
    __autoreleasing NSError* error = nil;
    id<MTLRenderPipelineState> pipeline = [mDevice->GetHandle() newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
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
    
    id<MTLDepthStencilState> depthStencilState = [mDevice->GetHandle() newDepthStencilStateWithDescriptor:depthStencilDesc];
    return depthStencilState;
}
#endif
