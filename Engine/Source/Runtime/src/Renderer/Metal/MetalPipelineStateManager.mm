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

static IRStaticSamplerDescriptor CreateStaticSampler(const SamplerState& samplerState)
{
    IRStaticSamplerDescriptor sampler{};
    sampler.MipLODBias = 0;
    sampler.MaxAnisotropy = 1;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = 16.0f;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = IRShaderVisibilityAll;
    sampler.ComparisonFunc = IRComparisonFunctionAlways;
    sampler.BorderColor = IRStaticBorderColorOpaqueBlack;
    
    switch (samplerState.filterMode)
    {
        case FilterMode::Point:
        {
            sampler.Filter = IRFilterMinMagMipPoint;
            break;
        }
        case FilterMode::Bilinear:
        {
            sampler.Filter = IRFilterMinMagLinearMipPoint;
            break;
        }
        case FilterMode::Trilinear:
        {
            sampler.Filter = IRFilterMinMagMipLinear;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Filter mode is not supported!") break;
    }

    switch (samplerState.wrapMode)
    {
        case WrapMode::Repeat:
        {
            sampler.AddressU = IRTextureAddressModeWrap;
            sampler.AddressV = IRTextureAddressModeWrap;
            sampler.AddressW = IRTextureAddressModeWrap;
            break;
        }
        case WrapMode::Clamp:
        {
            sampler.AddressU = IRTextureAddressModeClamp;
            sampler.AddressV = IRTextureAddressModeClamp;
            sampler.AddressW = IRTextureAddressModeClamp;
            break;
        }
        case WrapMode::Mirror:
        {
            sampler.AddressU = IRTextureAddressModeMirror;
            sampler.AddressV = IRTextureAddressModeMirror;
            sampler.AddressW = IRTextureAddressModeMirror;
            break;
        }
        case WrapMode::MirrorOnce:
        {
            sampler.AddressU = IRTextureAddressModeMirrorOnce;
            sampler.AddressV = IRTextureAddressModeMirrorOnce;
            sampler.AddressW = IRTextureAddressModeMirrorOnce;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Wrap mode is not supported!") break;
    }

    return sampler;
}

void MetalPipelineStateManager::Init(MetalDevice* device)
{
    mDevice = device;
    auto samplerSates = SamplerState::GetStaticSamplers();
    TArray<IRStaticSamplerDescriptor, samplerSates.size()> staticSamplerDescs{};
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        staticSamplerDescs[i] = CreateStaticSampler(samplerSates[i]);
        staticSamplerDescs[i].ShaderRegister = i;
    }
    
    // Root signature
    constexpr uint32_t NumRootParams = PUSH_CONSTANT_SLOT + 1;
    IRRootParameter1 rootSigParams[NumRootParams];
    for (uint32_t i = 0; i < PUSH_CONSTANT_SLOT; i++)
    {
        rootSigParams[i] = {
          .ParameterType = IRRootParameterTypeCBV,
          .Descriptor = {
              .ShaderRegister = i,
              .RegisterSpace = 0,
              .Flags = IRRootDescriptorFlagDataVolatile
          },
          .ShaderVisibility = IRShaderVisibilityAll
        };
    }
    // Push constant
    rootSigParams[PUSH_CONSTANT_SLOT] = {
      .ParameterType = IRRootParameterType32BitConstants,
      .Constants = {
          .ShaderRegister = PUSH_CONSTANT_REGISTER,
          .RegisterSpace = 0,
          .Num32BitValues = PUSH_CONSTANT_SIZE / sizeof(uint32_t)
      },
      .ShaderVisibility = IRShaderVisibilityAll
    };
    
    IRVersionedRootSignatureDescriptor rootSignature = {};
    rootSignature.version = IRRootSignatureVersion_1_1;
    rootSignature.desc_1_1.Flags = IRRootSignatureFlags(IRRootSignatureFlagDenyHullShaderRootAccess
                                                        | IRRootSignatureFlagDenyDomainShaderRootAccess
                                                        | IRRootSignatureFlagDenyGeometryShaderRootAccess
                                                        | IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed);

    rootSignature.desc_1_1.NumStaticSamplers = staticSamplerDescs.size();
    rootSignature.desc_1_1.pStaticSamplers = staticSamplerDescs.data();
    rootSignature.desc_1_1.pParameters = rootSigParams;
    rootSignature.desc_1_1.NumParameters = NumRootParams;
    
    IRError* pRootSigError = nullptr;
    mRootSignature = IRRootSignatureCreateFromDescriptor(&rootSignature, &pRootSigError);
    
    if (pRootSigError)
    {
        char* error_msg = (char*)IRErrorGetPayload(pRootSigError);
        GLEAM_CORE_ERROR("Metal: Root signature error: {0}\n", error_msg);
        IRErrorDestroy(pRootSigError);
    }
}

void MetalPipelineStateManager::Destroy()
{
    IRRootSignatureDestroy(mRootSignature);
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

IRRootSignature* MetalPipelineStateManager::GetGlobalRootSignature()
{
    return mRootSignature;
}

size_t MetalPipelineStateManager::GetTopLevelArgumentBufferSize()
{
    return PUSH_CONSTANT_SLOT * sizeof(uint64_t) + PUSH_CONSTANT_SIZE;
}
#endif
