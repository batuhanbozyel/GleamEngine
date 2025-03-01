#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "MetalUtils.h"
#include "MetalSwapchain.h"

#include "Core/Globals.h"
#include "Renderer/SamplerState.h"
#include "Renderer/RenderSystem.h"

#define IR_PRIVATE_IMPLEMENTATION
#include <metal_irconverter/metal_irconverter.h>
#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

void RenderSystem::InitializeBackend()
{
	mSwapchain = CreateScope<MetalSwapchain>();
    mReleaseQueue = CreateScope<ResourceReleaseQueue>(mSwapchain->GetFramesInFlight());
    
    mDevice = CreateScope<MetalDevice>(mSwapchain.get(), mReleaseQueue.get());
	mUploadManager = CreateScope<UploadManager>(mDevice.get());
	mResourcePool = CreateScope<RenderResourcePool>(mDevice.get(), mSwapchain.get(), mReleaseQueue.get());
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

MemoryRequirements GraphicsDevice::QueryMemoryRequirements(const HeapDescriptor& descriptor) const
{
	MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType);
    MTLSizeAndAlign sizeAndAlign = [mHandle heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];

    return MemoryRequirements
	{
		.size = sizeAndAlign.size,
		.alignment = sizeAndAlign.align
	};
}

Heap GraphicsDevice::CreateHeap(const HeapDescriptor& descriptor)
{
    Heap heap(descriptor);
    heap.mDevice = this;
    
    MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType) | MTLResourceHazardTrackingModeTracked; // TODO: Remove hazard tracking when async compute passes are implemented with proper resource synchronization
    MTLSizeAndAlign sizeAndAlign = [mHandle heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];
    
    MTLHeapDescriptor* desc = [MTLHeapDescriptor new];
    desc.type = MTLHeapTypePlacement;
    desc.resourceOptions = resourceOptions;
    desc.size = Utils::AlignUp(sizeAndAlign.size, sizeAndAlign.align);

    heap.mHandle = [mHandle newHeapWithDescriptor:desc];
    heap.mDescriptor.size = sizeAndAlign.size;
    heap.mAlignment = sizeAndAlign.align;
    
    [heap.mHandle setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:heap.mHandle];
    return heap;
}

Texture GraphicsDevice::CreateTexture(const TextureDescriptor& descriptor)
{
    Texture texture(descriptor);
    
    MTLTextureDescriptor* textureDesc;
    if (descriptor.dimension == TextureDimension::TextureCube)
    {
        float size = Math::Min(descriptor.size.width, descriptor.size.height);
        texture.mDescriptor.size.width = size;
        texture.mDescriptor.size.height = size;
        textureDesc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) size:size mipmapped:descriptor.useMipMap];
    }
    else
    {
        textureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:TextureFormatToMTLPixelFormat(descriptor.format) width:descriptor.size.width height:descriptor.size.height mipmapped:descriptor.useMipMap];
    }
    textureDesc.mipmapLevelCount = texture.mMipMapLevels;
    textureDesc.sampleCount = 1;
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.storageMode = MTLStorageModePrivate;
    texture.mHandle = [mHandle newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = texture.mHandle;
    texture.mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                                   textureType:descriptor.dimension == TextureDimension::TextureCube ? MTLTextureTypeCubeArray : MTLTextureType2DArray
                                                        levels:NSMakeRange(0, texture.mMipMapLevels)
                                                        slices:NSMakeRange(0, 1)];
    [baseTexture setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [texture.mView setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:texture.mHandle];
    texture.mResourceView = Utils::IsDepthFormat(descriptor.format) ? InvalidResourceIndex : CreateResourceView(texture);
    return texture;
}

Shader GraphicsDevice::CompileShader(const TString& entryPoint, ShaderStage stage)
{
    Shader shader(entryPoint, stage);
    auto shaderPath = Globals::BuiltinAssetsDirectory/"Shaders";
    File shaderFile = Filesystem::Open(shaderPath.append(entryPoint + ".dxil"), FileType::Binary);
    auto shaderCode = shaderFile.Read();
    auto dxil = IRObjectCreateFromDXIL((uint8_t*)shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipNone);
    
    auto compiler = IRCompilerCreate();
    IRCompilerSetEntryPointName(compiler, entryPoint.data());
    IRCompilerSetMinimumDeploymentTarget(compiler, IROperatingSystem_macOS, "14.0");
    IRCompilerSetGlobalRootSignature(compiler, static_cast<MetalDevice*>(this)->GetGlobalRootSignature());
    
    IRError* compileError = nullptr;
    auto metalIR = IRCompilerAllocCompileAndLink(compiler, entryPoint.c_str(), dxil, &compileError);
    if (compileError)
    {
        auto errorCode = IRErrorGetCode(compileError);
        GLEAM_CORE_ERROR("Metal IR generation failed with code: {0}", errorCode);
        IRErrorDestroy(compileError);
        compileError = nullptr;
    }
    
    IRMetalLibBinary* metallibBinary = IRMetalLibBinaryCreate();
    IRObjectGetMetalLibBinary(metalIR, IRObjectGetMetalIRShaderStage(metalIR), metallibBinary);
    dispatch_data_t data = IRMetalLibGetBytecodeData(metallibBinary);
    
    NSError* __autoreleasing libraryError = nil;
    id<MTLLibrary> library = [mHandle newLibraryWithData:data error:&libraryError];
    if (libraryError)
    {
        auto errorStr = TO_CPP_STRING([libraryError localizedDescription]);
        GLEAM_CORE_ERROR("Metal library load failed: {0}", errorStr);
        libraryError = nil;
    }
    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    shader.mHandle = [library newFunctionWithName:functionName];
    
    // Clean up
    IRMetalLibBinaryDestroy(metallibBinary);
    IRObjectDestroy(dxil);
    IRObjectDestroy(metalIR);
    IRCompilerDestroy(compiler);
    
    return shader;
}

GraphicsPipeline GraphicsDevice::CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc)
{
    GraphicsPipeline pipeline(pipelineDesc);
    pipeline.mHandle = (__bridge id)(new MetalGraphicsPipeline);
    
    auto mtlPipeline = (__bridge MetalGraphicsPipeline*)(pipeline.mHandle);
    auto vertexShader = CreateShader(pipelineDesc.vertexEntry, ShaderStage::Vertex);
    auto fragmentShader = CreateShader(pipelineDesc.fragmentEntry, ShaderStage::Fragment);
    
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = 1;
    pipelineDescriptor.vertexFunction = vertexShader.GetHandle();
    pipelineDescriptor.fragmentFunction = fragmentShader.GetHandle();
    pipelineDescriptor.alphaToCoverageEnabled = pipelineDesc.alphaToCoverage;
    pipelineDescriptor.inputPrimitiveTopology = PrimitiveTopologyToMTLPrimitiveTopologyClass(pipelineDesc.topology);
    for (uint32_t i = 0; i < pipelineDesc.colorFormats.size(); i++)
    {
        pipelineDescriptor.colorAttachments[i].pixelFormat = TextureFormatToMTLPixelFormat(pipelineDesc.colorFormats[i]);
        pipelineDescriptor.colorAttachments[i].blendingEnabled = pipelineDesc.blendState.enabled;
        pipelineDescriptor.colorAttachments[i].sourceRGBBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.sourceColorBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationRGBBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.destinationColorBlendMode);
        pipelineDescriptor.colorAttachments[i].sourceAlphaBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.sourceAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].destinationAlphaBlendFactor = BlendModeToMTLBlendFactor(pipelineDesc.blendState.destinationAlphaBlendMode);
        pipelineDescriptor.colorAttachments[i].rgbBlendOperation = BlendOpToMTLBlendOperation(pipelineDesc.blendState.colorBlendOperation);
        pipelineDescriptor.colorAttachments[i].alphaBlendOperation = BlendOpToMTLBlendOperation(pipelineDesc.blendState.alphaBlendOperation);
        pipelineDescriptor.colorAttachments[i].writeMask = ColorWriteMaskToMTLColorWriteMask(pipelineDesc.blendState.writeMask);
    }

    if (Utils::IsDepthFormat(pipelineDesc.depthFormat))
    {
        MTLPixelFormat format = TextureFormatToMTLPixelFormat(pipelineDesc.depthFormat);
        pipelineDescriptor.depthAttachmentPixelFormat = format;
        
        MTLDepthStencilDescriptor* depthStencilDesc = [MTLDepthStencilDescriptor new];
        depthStencilDesc.depthWriteEnabled = pipelineDesc.depthState.writeEnabled;
        depthStencilDesc.depthCompareFunction = CompareFunctionToMTLCompareFunction(pipelineDesc.depthState.compareFunction);
        
        if (pipelineDesc.stencilState.enabled)
        {
            pipelineDescriptor.stencilAttachmentPixelFormat = format;
            
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
        mtlPipeline->depthStencilState = [mHandle newDepthStencilStateWithDescriptor:depthStencilDesc];
        GLEAM_ASSERT(mtlPipeline->depthStencilState, "Metal: Graphics Pipeline depth state creation failed.");
    }
    
    __autoreleasing NSError* error = nil;
    mtlPipeline->renderState = [mHandle newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    mtlPipeline->topology = PrimitiveTopologyToMTLPrimitiveType(pipelineDesc.topology);
    GLEAM_ASSERT(mtlPipeline->renderState, "Metal: Graphics Pipeline render state creation failed.");
    return pipeline;
}

void GraphicsDevice::Dispose(Heap& heap)
{
    [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:heap.mHandle];
    heap.mHandle = nil;
}

void GraphicsDevice::Dispose(Texture& texture)
{
    [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:texture.mHandle];
    ReleaseResourceView(texture.mResourceView);
    texture.mHandle = nil;
    texture.mView = nil;
}

void GraphicsDevice::Dispose(Shader& shader)
{
	shader.mHandle = nil;
}

void GraphicsDevice::Dispose(GraphicsPipeline& pipeline)
{
    auto mtlPipeline = (__bridge MetalGraphicsPipeline*)(pipeline.mHandle);
    delete mtlPipeline;
    pipeline.mHandle = nullptr;
}

MetalDevice::MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue)
    : GraphicsDevice(surface, releaseQueue)
{
    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);
    
    // init MTLResidencySet
    __autoreleasing NSError* error = nil;
    MTLResidencySetDescriptor* residencySetDesc = [MTLResidencySetDescriptor new];
    residencySetDesc.initialCapacity = 1024;
    mResidencySet = [mHandle newResidencySetWithDescriptor:residencySetDesc error:&error];
    GLEAM_ASSERT(mResidencySet, "Metal: Residency set creation failed.");
    
    // init MTLCommandQueue
    mCommandPool = [mHandle newCommandQueue];
    [mCommandPool addResidencySet:mResidencySet];
    
    // create descriptor heap
    mCbvSrvUavHeap = CreateDescriptorHeap(CBV_SRV_HEAP_SIZE);

    auto samplerSates = SamplerState::GetStaticSamplers();
    TArray<IRStaticSamplerDescriptor, samplerSates.size()> staticSamplerDescs{};
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        staticSamplerDescs[i] = CreateStaticSampler(samplerSates[i]);
        staticSamplerDescs[i].ShaderRegister = i;
    }
    
    // root signature
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

    GLEAM_CORE_INFO("Metal: Graphics device created.");
}

MetalDevice::~MetalDevice()
{
    mShaderCache.clear();
    IRRootSignatureDestroy(mRootSignature);
    
    // Destroy descriptor heap
    mCbvSrvUavHeap.handle = nil;
    
    // Destroy residency set
    [mCommandPool removeResidencySet:mResidencySet];
    mResidencySet = nil;

    // Destroy command pool
    mCommandPool = nil;

    // Destroy device
    mHandle = nil;

    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

void MetalDevice::Configure(const RendererConfig& config)
{
    auto swapchain = static_cast<MetalSwapchain*>(mSurface);
    swapchain->Configure(this, config);
}

ShaderResourceIndex MetalDevice::CreateResourceView(const Buffer& buffer)
{
    auto index = mCbvSrvUavHeap.heap.Allocate();
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    IRDescriptorTableSetBuffer(descriptorTable + index.data, [buffer.GetHandle() gpuAddress], 0);
    return index;
}

ShaderResourceIndex MetalDevice::CreateResourceView(const Texture& texture)
{
    auto index = mCbvSrvUavHeap.heap.Allocate();
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    IRDescriptorTableSetTexture(descriptorTable + index.data, texture.GetRenderTargetView(), 0.0f, 0);
    return index;
}

void MetalDevice::ReleaseResourceView(ShaderResourceIndex view)
{
    if (view != InvalidResourceIndex)
    {
        mCbvSrvUavHeap.heap.Release(view);
    }
}

MetalDescriptorHeap MetalDevice::CreateDescriptorHeap(uint32_t capacity) const
{
    MetalDescriptorHeap heap;
    heap.handle = [mHandle newBufferWithLength:capacity * sizeof(IRDescriptorTableEntry) options:MTLResourceStorageModeShared];
    heap.heap = ResourceDescriptorHeap(capacity);
    
    [heap.handle setLabel:@"DescriptorHeap"];
    return heap;
}

id<MTLBuffer> MetalDevice::GetCbvSrvUavHeap() const
{
    return mCbvSrvUavHeap.handle;
}

id<MTLCommandQueue> MetalDevice::GetCommandPool() const
{
    return mCommandPool;
}

id<MTLResidencySet> MetalDevice::GetResidencySet() const
{
    return mResidencySet;
}

id<MTLCommandBuffer> MetalDevice::AllocateCommandBuffer() const
{
    [mResidencySet commit];
#ifdef GDEBUG
    MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
    descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
    return [mCommandPool commandBufferWithDescriptor:descriptor];
#else
    return [mCommandPool commandBuffer];
#endif
}

IRRootSignature* MetalDevice::GetGlobalRootSignature() const
{
    return mRootSignature;
}

#endif
