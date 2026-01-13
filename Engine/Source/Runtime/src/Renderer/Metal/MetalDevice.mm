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

@interface MetalComputePipelineImpl : NSObject<MetalComputePipeline>
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineState;
@property(nonatomic, assign) MTLSize threadsPerThreadgroup;
@end

@implementation MetalComputePipelineImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _pipelineState = nil;
        _threadsPerThreadgroup = MTLSizeMake(1, 1, 1);
    }
    return self;
}

@end

@interface MetalGraphicsPipelineImpl : NSObject<MetalGraphicsPipeline>
@property (nonatomic, strong) id<MTLRenderPipelineState> pipelineState;
@property (nonatomic, strong) id<MTLDepthStencilState> depthStencilState;
@property (nonatomic, assign) MTLPrimitiveType topology;
@end

@implementation MetalGraphicsPipelineImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _pipelineState = nil;
        _depthStencilState = nil;
        _topology = MTLPrimitiveTypeTriangle;
    }
    return self;
}

@end

@interface MetalFunctionImpl : NSObject<MetalFunction>
@property (nonatomic, strong) id<MTLFunction> function;
@end

@implementation MetalFunctionImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _function = nil;
    }
    return self;
}

@end

@interface MetalComputeFunctionImpl : MetalFunctionImpl<MetalComputeFunction>
@property (nonatomic, assign) MTLSize threadsPerThreadgroup;
@end

@implementation MetalComputeFunctionImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _threadsPerThreadgroup = MTLSizeMake(1, 1, 1);
    }
    return self;
}

@end

void RenderSystem::InitializeBackend()
{
	mSwapchain = CreateScope<MetalSwapchain>();
    mReleaseQueue = CreateScope<ResourceReleaseQueue>(mSwapchain->GetFramesInFlight());
    
    mDevice = CreateScope<MetalDevice>(mSwapchain.get(), mReleaseQueue.get());
	mCopyCommandBuffer = CreateScope<CopyCommandBuffer>(mDevice.get());

    mPersistentAllocator = CreateScope<GPUAllocator>(mDevice.get(), GPUAllocatorDescriptor{ .name = "Persistent GPU Allocator" });
	mTransientAllocator = CreateScope<GPUAllocator>(mDevice.get(), GPUAllocatorDescriptor{ .name = "Transient GPU Allocator" });
}

static IRStaticSamplerDescriptor CreateIRStaticSampler(const SamplerState& samplerState)
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
        default: GLEAM_ASSERT(false, "Metal: Filter mode is not supported.") break;
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
        default: GLEAM_ASSERT(false, "Metal: Wrap mode is not supported.") break;
    }
    return sampler;
}

static MTLSamplerDescriptor* CreateMTLSamplerState(const SamplerState& samplerState)
{
    MTLSamplerDescriptor* desc = [MTLSamplerDescriptor new];
    desc.supportArgumentBuffers = true;
    desc.maxAnisotropy = 1;
    desc.lodMinClamp = 0.0f;
    desc.lodMaxClamp = 16.0f;
    desc.compareFunction = MTLCompareFunctionAlways;
    
    switch (samplerState.filterMode)
    {
        case FilterMode::Point:
        {
            desc.minFilter = MTLSamplerMinMagFilterNearest;
            desc.magFilter = MTLSamplerMinMagFilterNearest;
            desc.mipFilter = MTLSamplerMipFilterNearest;
            break;
        }
        case FilterMode::Bilinear:
        {
            desc.minFilter = MTLSamplerMinMagFilterLinear;
            desc.magFilter = MTLSamplerMinMagFilterLinear;
            desc.mipFilter = MTLSamplerMipFilterNearest;
            break;
        }
        case FilterMode::Trilinear:
        {
            desc.minFilter = MTLSamplerMinMagFilterLinear;
            desc.magFilter = MTLSamplerMinMagFilterLinear;
            desc.mipFilter = MTLSamplerMipFilterLinear;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Filter mode is not supported.") break;
    }
    
    switch (samplerState.wrapMode)
    {
        case WrapMode::Repeat:
        {
            desc.sAddressMode = MTLSamplerAddressModeRepeat;
            desc.tAddressMode = MTLSamplerAddressModeRepeat;
            desc.rAddressMode = MTLSamplerAddressModeRepeat;
            break;
        }
        case WrapMode::Clamp:
        {
            desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
            desc.tAddressMode = MTLSamplerAddressModeClampToEdge;
            desc.rAddressMode = MTLSamplerAddressModeClampToEdge;
            break;
        }
        case WrapMode::Mirror:
        {
            desc.sAddressMode = MTLSamplerAddressModeMirrorRepeat;
            desc.tAddressMode = MTLSamplerAddressModeMirrorRepeat;
            desc.rAddressMode = MTLSamplerAddressModeMirrorRepeat;
            break;
        }
        case WrapMode::MirrorOnce:
        {
            desc.sAddressMode = MTLSamplerAddressModeMirrorClampToEdge;
            desc.tAddressMode = MTLSamplerAddressModeMirrorClampToEdge;
            desc.rAddressMode = MTLSamplerAddressModeMirrorClampToEdge;
            break;
        }
        default: GLEAM_ASSERT(false, "Metal: Wrap mode is not supported.") break;
    }
    return desc;
}

Heap GraphicsDevice::CreateHeap(const HeapDescriptor& descriptor)
{
    MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType);
    MTLSizeAndAlign sizeAndAlign = [mHandle heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];
    
    MTLHeapDescriptor* desc = [MTLHeapDescriptor new];
    desc.type = MTLHeapTypePlacement;
    desc.resourceOptions = resourceOptions;
    desc.size = Utils::AlignUp(sizeAndAlign.size, sizeAndAlign.align);

    Heap heap(descriptor);
    heap.mHandle = [mHandle newHeapWithDescriptor:desc];
    heap.mDescriptor.size = sizeAndAlign.size;
    heap.mAlignment = sizeAndAlign.align;
    
    [heap.mHandle setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:heap.mHandle];
    return heap;
}

Texture GraphicsDevice::CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor)
{
    Texture texture(descriptor);
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor new];
    textureDesc.pixelFormat = TextureFormatToMTLPixelFormat(descriptor.format);
    textureDesc.width = descriptor.size.width;
    textureDesc.height = descriptor.size.height;
    textureDesc.mipmapLevelCount = texture.mMipMapLevels;
    textureDesc.arrayLength = 1;
    textureDesc.sampleCount = 1;
    textureDesc.depth = 1;
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.storageMode = MTLStorageModePrivate; // TODO: add support for cpu visible textures

    NSUInteger sliceCount = descriptor.depth;
    switch (descriptor.dimension)
    {
        case TextureDimension::Texture2D:
        {
            if (descriptor.depth == 1)
            {
                textureDesc.textureType = MTLTextureType2D;
            }
            else
            {
                textureDesc.arrayLength = descriptor.depth;
                textureDesc.textureType = MTLTextureType2DArray;
            }
            break;
        }
        case TextureDimension::Texture3D:
        {
            textureDesc.textureType = MTLTextureType3D;
            textureDesc.depth = descriptor.depth;
            break;
        }
        case TextureDimension::TextureCube:
        {
            sliceCount = 6 * descriptor.depth;
            textureDesc.arrayLength = descriptor.depth;
            if (descriptor.depth == 1)
            {
                textureDesc.textureType = MTLTextureTypeCube;
            }
            else
            {
                textureDesc.textureType = MTLTextureTypeCubeArray;
            }
            break;
        }
    }

    MTLSizeAndAlign sizeAndAlign = [mHandle heapTextureSizeAndAlignWithDescriptor:textureDesc];
    MemoryRequirements memoryRequirements =
	{
		.size = sizeAndAlign.size,
		.alignment = sizeAndAlign.align,
		.type = MemoryType::GPU
	};
    GPUAllocation allocation = allocator->Allocate(memoryRequirements);

    id<MTLHeap> heap = allocation.block->heap.GetHandle();
    id<MTLTexture> baseTexture = [heap newTextureWithDescriptor:textureDesc offset:allocation.offset];
    [baseTexture setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:baseTexture];
    allocator->AddAllocation(baseTexture, allocation);
    texture.mHandle = baseTexture;
    
    // Create main view
    {
        MTLTextureViewDescriptor* viewDesc = [MTLTextureViewDescriptor new];
        viewDesc.pixelFormat = textureDesc.pixelFormat;
        viewDesc.textureType = textureDesc.textureType;
        viewDesc.levelRange = NSMakeRange(0, textureDesc.mipmapLevelCount);
        viewDesc.sliceRange = NSMakeRange(0, sliceCount);
        viewDesc.swizzle = textureDesc.swizzle;
        texture.mResourceView = static_cast<MetalDevice*>(this)->CreateResourceView(texture, viewDesc);
    }
    
    for (uint32_t i = 0; i < texture.mSliceUnorderedAccessViews.size(); ++i)
    {
        uint32_t slice = i / texture.GetMipMapLevels();
        uint32_t mip = i % texture.GetMipMapLevels();
        
        MTLTextureViewDescriptor* viewDesc = [MTLTextureViewDescriptor new];
        viewDesc.pixelFormat = textureDesc.pixelFormat;
        
        switch (descriptor.dimension)
        {
            case TextureDimension::Texture2D:
            case TextureDimension::TextureCube:
            {
                viewDesc.textureType = MTLTextureType2DArray;
                break;
            }
            case TextureDimension::Texture3D:
            {
                viewDesc.textureType = MTLTextureType3D;
                break;
            }
        }
        viewDesc.levelRange = NSMakeRange(mip, 1);
        viewDesc.sliceRange = NSMakeRange(slice, 1);
        viewDesc.swizzle = textureDesc.swizzle;
        texture.mSliceUnorderedAccessViews[i] = static_cast<MetalDevice*>(this)->CreateResourceView(texture, viewDesc);
    }
    
    // Create RTV for attachments
    if (descriptor.usage & TextureUsage_Attachment)
    {
        auto descriptorTable = static_cast<IRDescriptorTableEntry*>([static_cast<MetalDevice*>(this)->GetCbvSrvUavHeap() contents]);
        // Create main RTV
        {
            auto entry = descriptorTable + texture.mResourceView.data;
            texture.mView._impl = entry->textureViewID;
        }
        
        // Create slice RTV
        for (uint32_t i = 0; i < texture.mSliceViews.size(); i++)
        {
            uint32_t slice = i / texture.GetMipMapLevels();
            uint32_t mip = i % texture.GetMipMapLevels();
            
            auto uav = texture.GetUnorderedAccessView(mip, slice);
            auto entry = descriptorTable + uav.data;
            texture.mSliceViews[i]._impl = entry->textureViewID;
        }
    }
    return texture;
}

Buffer GraphicsDevice::CreateBuffer(GPUAllocator* allocator, const BufferDescriptor& descriptor)
{
    MTLResourceOptions resourceOptions = MemoryTypeToMTLResourceOption(descriptor.memoryType);
    MTLSizeAndAlign sizeAndAlign = [mHandle heapBufferSizeAndAlignWithLength:descriptor.size options:resourceOptions];
    MemoryRequirements memoryRequirements =
	{
		.size = sizeAndAlign.size,
		.alignment = sizeAndAlign.align,
		.type = descriptor.memoryType
	};
    GPUAllocation allocation = allocator->Allocate(memoryRequirements);

    id<MTLHeap> heap = allocation.block->heap.GetHandle();
    id<MTLBuffer> mtlBuffer = [heap newBufferWithLength:descriptor.size options:heap.resourceOptions offset:allocation.offset];
    [mtlBuffer setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:mtlBuffer];
    allocator->AddAllocation(mtlBuffer, allocation);

    void* contents = nullptr;
    if (allocation.block->heap.GetDescriptor().memoryType != MemoryType::GPU)
    {
        contents = [mtlBuffer contents];
    }
    
    Buffer buffer(descriptor);
    buffer.mHandle = mtlBuffer;
    buffer.mContents = contents;
    buffer.mAlignment = 4;
    buffer.mResourceView = static_cast<MetalDevice*>(this)->CreateResourceView(buffer);
    return buffer;
}

Shader GraphicsDevice::CompileShader(const TString& entryPoint, ShaderStage stage)
{
    Shader shader(entryPoint, stage);
    auto shaderPath = Globals::BuiltinAssetsDirectory/"Shaders";
    File shaderFile = Filesystem::Open(shaderPath.Append(entryPoint + ".dxil"), FileType::Binary);
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
    
    __autoreleasing NSError* libraryError = nil;
    id<MTLLibrary> library = [mHandle newLibraryWithData:data error:&libraryError];
    if (libraryError)
    {
        auto errorStr = TO_CPP_STRING([libraryError localizedDescription]);
        GLEAM_CORE_ERROR("Metal library load failed: {0}", errorStr);
        libraryError = nil;
    }
    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    id<MTLFunction> mtlFunction = [library newFunctionWithName:functionName];
    
    if (stage == ShaderStage::Compute)
    {
        MetalComputeFunctionImpl* computeFunction = [[MetalComputeFunctionImpl alloc] init];
        computeFunction.function = mtlFunction;
        
        IRShaderReflection* reflection = IRShaderReflectionCreate();
        IRObjectGetReflection(metalIR, IRShaderStageCompute, reflection);
        
        IRVersionedCSInfo csInfo;
        IRShaderReflectionCopyComputeInfo(reflection, IRReflectionVersion_1_0, &csInfo);
        computeFunction.threadsPerThreadgroup = MTLSizeMake(csInfo.info_1_0.tg_size[0], csInfo.info_1_0.tg_size[1], csInfo.info_1_0.tg_size[2]);
        shader.mHandle = computeFunction;
        
        IRShaderReflectionReleaseComputeInfo(&csInfo);
        IRShaderReflectionDestroy(reflection);
    }
    else
    {
        MetalFunctionImpl* baseFunction = [[MetalFunctionImpl alloc] init];
        baseFunction.function = mtlFunction;
        shader.mHandle = baseFunction;
    }
    
    // Clean up
    IRMetalLibBinaryDestroy(metallibBinary);
    IRObjectDestroy(dxil);
    IRObjectDestroy(metalIR);
    IRCompilerDestroy(compiler);
    
    return shader;
}

ComputePipeline GraphicsDevice::CompileComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc)
{
    ComputePipeline pipeline(pipelineDesc);
    pipeline.mHandle = [[MetalComputePipelineImpl alloc] init];
    
	auto shader = CreateShader(pipelineDesc.entryPoint, ShaderStage::Compute);
    id<MetalComputeFunction> mtlFunction = shader.GetHandle();
    id<MetalComputePipeline> mtlPipeline = pipeline.mHandle;
    
    __autoreleasing NSError* error = nil;
    mtlPipeline.pipelineState = [mHandle newComputePipelineStateWithFunction:mtlFunction.function error:&error];
    mtlPipeline.threadsPerThreadgroup = mtlFunction.threadsPerThreadgroup;
    GLEAM_ASSERT(mtlPipeline.pipelineState, "Metal: Compute pipeline state creation failed.");
    return pipeline;
}

GraphicsPipeline GraphicsDevice::CompileGraphicsPipeline(const GraphicsPipelineStateDescriptor& pipelineDesc)
{
    GraphicsPipeline pipeline(pipelineDesc);
    pipeline.mHandle = [[MetalGraphicsPipelineImpl alloc] init];
    
    id<MetalGraphicsPipeline> mtlPipeline = pipeline.mHandle;
    auto vertexShader = CreateShader(pipelineDesc.vertexEntry, ShaderStage::Vertex);
    auto fragmentShader = CreateShader(pipelineDesc.fragmentEntry, ShaderStage::Fragment);
    
    id<MetalFunction> vertexFunction = vertexShader.GetHandle();
    id<MetalFunction> fragmentFunction = fragmentShader.GetHandle();
    
    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = 1;
    pipelineDescriptor.vertexFunction = vertexFunction.function;
    pipelineDescriptor.fragmentFunction = fragmentFunction.function;
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
        mtlPipeline.depthStencilState = [mHandle newDepthStencilStateWithDescriptor:depthStencilDesc];
        GLEAM_ASSERT(mtlPipeline.depthStencilState, "Metal: Graphics Pipeline depth state creation failed.");
    }
    
    __autoreleasing NSError* error = nil;
    mtlPipeline.pipelineState = [mHandle newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
    mtlPipeline.topology = PrimitiveTopologyToMTLPrimitiveType(pipelineDesc.topology);
    GLEAM_ASSERT(mtlPipeline.pipelineState, "Metal: Graphics Pipeline render state creation failed.");
    return pipeline;
}

void GraphicsDevice::Dispose(Heap& heap)
{
    [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:heap.mHandle];
    heap.mHandle = nil;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Buffer& buffer)
{
    const auto& allocation = allocator->GetAllocation(buffer.GetHandle());
	allocator->Free(allocation);

    id<MTLBuffer> resource = buffer.GetHandle();
    ShaderResourceIndex view = buffer.GetResourceView();
    mReleaseQueue->AddResource([this, resource, view]()
    {
        [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
        static_cast<MetalDevice*>(this)->ReleaseResourceView(view);
    }, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
    
	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nil;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Texture& texture)
{
    const auto& allocation = allocator->GetAllocation(texture.GetHandle());
	allocator->Free(allocation);
    
    mReleaseQueue->AddResource([this,
                                resource = texture.GetHandle(),
                                rtv = texture.GetRenderTargetView(),
                                view = texture.GetResourceView(),
                                usage = texture.GetDescriptor().usage,
                                sliceUnorderedViews = texture.mSliceUnorderedAccessViews]()
    {
        [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
        static_cast<MetalDevice*>(this)->ReleaseResourceView(view);
        
        // we dont need to release RTVs since they are identical with UAV
        if (usage & TextureUsage_Attachment)
        {
            // noop
        }
        
        // Release slice resource views
        for (const auto& unorderedView : sliceUnorderedViews)
        {
            static_cast<MetalDevice*>(this)->ReleaseResourceView(unorderedView);
        }
    }, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
    
    texture.mResourceView = InvalidResourceIndex;
    texture.mHandle = nil;
    texture.mView = {};
    texture.mSliceViews.clear();
    texture.mSliceUnorderedAccessViews.clear();
}

void GraphicsDevice::Dispose(Shader& shader)
{
	shader.mHandle = nil;
}

void GraphicsDevice::Dispose(ComputePipeline& pipeline)
{
    pipeline.mHandle = nil;
}

void GraphicsDevice::Dispose(GraphicsPipeline& pipeline)
{
    pipeline.mHandle = nil;
}

MetalDevice::MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue)
    : GraphicsDevice(surface, releaseQueue)
{
    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);
    
    // init MTLResidencySet
    __autoreleasing NSError* residencySetError = nil;
    MTLResidencySetDescriptor* residencySetDesc = [MTLResidencySetDescriptor new];
    residencySetDesc.initialCapacity = CBV_SRV_HEAP_SIZE;
    residencySetDesc.label = @"ResidencySet";
    mResidencySet = [mHandle newResidencySetWithDescriptor:residencySetDesc error:&residencySetError];
    GLEAM_ASSERT(mResidencySet, "Metal: Residency set creation failed.");
    
    // init MTLCommandQueue
    mCommandQueue = [mHandle newMTL4CommandQueue];
    [mCommandQueue addResidencySet:mResidencySet];
    
    // create descriptor heap
    uint32_t maxSamplers = (uint32_t)[mHandle maxArgumentBufferSamplerCount];
    mSamplerHeap = CreateSamplerHeap(Math::Clamp(0u, maxSamplers, 1024u));
    mCbvSrvUavHeap = CreateDescriptorHeap(CBV_SRV_HEAP_SIZE);

    auto samplerSates = SamplerState::GetStaticSamplers();
    mStaticSamplers.resize(samplerSates.size());
    
    for (uint32_t i = 0; i < samplerSates.size(); i++)
    {
        mStaticSamplers[i] = (__bridge_retained void*)CreateSampler(samplerSates[i]);
    }
    
    // root signature
    constexpr uint32_t NumRootParams = PUSH_CONSTANT_SLOT + 2; // 1 for samplers descriptor table, 1 for push constants
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
    
    // Sampler descriptor table
    IRDescriptorRange1 samplerRange = {
        .RangeType = IRDescriptorRangeTypeSampler,
        .NumDescriptors = (uint32_t)mStaticSamplers.size(),
        .BaseShaderRegister = 0,
        .RegisterSpace = 0,
        .Flags = IRDescriptorRangeFlagNone,
        .OffsetInDescriptorsFromTableStart = 0
    };
    rootSigParams[PUSH_CONSTANT_SLOT + 1] = {
        .ParameterType = IRRootParameterTypeDescriptorTable,
        .DescriptorTable = {
            .NumDescriptorRanges = 1,
            .pDescriptorRanges = &samplerRange
        },
        .ShaderVisibility = IRShaderVisibilityAll
    };
    
    IRVersionedRootSignatureDescriptor rootSignature = {};
    rootSignature.version = IRRootSignatureVersion_1_1;
    rootSignature.desc_1_1.Flags = IRRootSignatureFlags(IRRootSignatureFlagDenyHullShaderRootAccess
                                                        | IRRootSignatureFlagDenyDomainShaderRootAccess
                                                        | IRRootSignatureFlagDenyGeometryShaderRootAccess
                                                        | IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed);

    rootSignature.desc_1_1.NumStaticSamplers = 0;
    rootSignature.desc_1_1.pStaticSamplers = nullptr;
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
    
    MTL4ArgumentTableDescriptor* argumentTableDesc = [MTL4ArgumentTableDescriptor new];
    argumentTableDesc.maxBufferBindCount = MaxArgumentTableBufferCount;
    argumentTableDesc.label = @"ArgumentTable";
    
    __autoreleasing NSError* argumentTableError = nil;
    mArgumentTable = [mHandle newArgumentTableWithDescriptor:argumentTableDesc error:&argumentTableError];
    [mArgumentTable setAddress:[mCbvSrvUavHeap.handle gpuAddress] atIndex:kIRDescriptorHeapBindPoint];
    [mArgumentTable setAddress:[mSamplerHeap.handle gpuAddress] atIndex:kIRSamplerHeapBindPoint];

    GLEAM_CORE_INFO("Metal: Graphics device created.");
}

MetalDevice::~MetalDevice()
{
    mShaderCache.clear();
    mComputePipelineCache.clear();
    mGraphicsPipelineCache.clear();
    IRRootSignatureDestroy(mRootSignature);
    
    // Destroy descriptor heap
    for (void* sampler : mStaticSamplers)
    {
        CFRelease(sampler);
    }
    mStaticSamplers.clear();
    mSamplerHeap.handle = nil;
    mCbvSrvUavHeap.pool = nil;
    mCbvSrvUavHeap.handle = nil;
    
    // Destroy residency set
    [mCommandQueue removeResidencySet:mResidencySet];
    mResidencySet = nil;
    mArgumentTable = nil;

    // Destroy command queue
    for (auto& pool : mCommandPools)
    {
        pool.Release();
    }
    mCommandPools.clear();
    mCommandQueue = nil;

    // Destroy device
    mHandle = nil;

    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

void MetalDevice::Configure(const RendererConfig& config)
{
    auto swapchain = static_cast<MetalSwapchain*>(mSurface);
    swapchain->Configure(this, config);
    
    for (auto& pool : mCommandPools)
    {
        pool.Release();
    }
    mCommandPools.clear();
    
    mCommandPools.resize(swapchain->mMaxFramesInFlight);
    for (uint32_t i = 0; i < swapchain->mMaxFramesInFlight; i++)
    {
        auto& pool = mCommandPools[i];
        
        TStringStream ss;
        ss << "CommandAllocator[" << swapchain->mCurrentFrameIndex << "]";
        TString cmdAllocatorName = ss.str();
        
        MTL4CommandAllocatorDescriptor* descriptor = [MTL4CommandAllocatorDescriptor new];
        descriptor.label = TO_NSSTRING(cmdAllocatorName.c_str());
        
        __autoreleasing NSError* error = nil;
        pool.allocator = [mHandle newCommandAllocatorWithDescriptor:descriptor error:&error];
        GLEAM_ASSERT(pool.allocator, "Metal: Command allocator creation failed.");
    }
}

void MetalDevice::ResetCommandPools(uint32_t frameIndex)
{
    mCommandPools[frameIndex].Reset();
}

ShaderResourceIndex MetalDevice::CreateResourceView(const Buffer& buffer)
{
    auto index = mCbvSrvUavHeap.heap.Allocate();
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    IRDescriptorTableSetBuffer(descriptorTable + index.data, [buffer.GetHandle() gpuAddress], 0);
    return index;
}

ShaderResourceIndex MetalDevice::CreateResourceView(const Texture& texture, MTLTextureViewDescriptor* viewDesc)
{
    auto index = mCbvSrvUavHeap.heap.Allocate();
    auto resourceID = [static_cast<MetalDevice*>(this)->GetRtvHeap() setTextureView:texture.GetHandle() descriptor:viewDesc atIndex:index.data];
    
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    auto entry = descriptorTable + index.data;
    
    entry->gpuVA = 0;
    entry->textureViewID = texture.GetRenderTargetView()._impl;
    entry->metadata = 0;
    
    return index;
}

void MetalDevice::ReleaseResourceView(ShaderResourceIndex view)
{
    if (view != InvalidResourceIndex)
    {
        mCbvSrvUavHeap.heap.Release(view);
    }
}

id<MTLSamplerState> MetalDevice::CreateSampler(const SamplerState& samplerState)
{
    MTLSamplerDescriptor* mtlSamplerDesc = CreateMTLSamplerState(samplerState);
    id<MTLSamplerState> mtlSampler = [mHandle newSamplerStateWithDescriptor:mtlSamplerDesc];
    
    auto index = mSamplerHeap.heap.Allocate();
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mSamplerHeap.handle contents]);
    IRDescriptorTableSetSampler(descriptorTable + index.data, mtlSampler, 0.0f);
    
    return mtlSampler;
}

MetalDescriptorHeap MetalDevice::CreateSamplerHeap(uint32_t capacity) const
{
    MetalDescriptorHeap heap;
    heap.handle = [mHandle newBufferWithLength:capacity * sizeof(IRDescriptorTableEntry) options:MTLResourceStorageModeShared];
    heap.heap = ResourceDescriptorHeap(capacity);
    [heap.handle setLabel:@"SamplerHeap"];
    return heap;
}

MetalDescriptorHeap MetalDevice::CreateDescriptorHeap(uint32_t capacity) const
{
    MetalDescriptorHeap heap;
    heap.handle = [mHandle newBufferWithLength:capacity * sizeof(IRDescriptorTableEntry) options:MTLResourceStorageModeShared];
    heap.heap = ResourceDescriptorHeap(capacity);
    [heap.handle setLabel:@"DescriptorHeap"];
    
    __autoreleasing NSError* error = nil;
    MTLResourceViewPoolDescriptor* desc = [MTLResourceViewPoolDescriptor new];
    desc.resourceViewCount = capacity;
    desc.label = @"TextureViewPool";
    heap.pool = [mHandle newTextureViewPoolWithDescriptor:desc error:&error];
    return heap;
}

id<MTLBuffer> MetalDevice::GetSamplerHeap() const
{
    return mSamplerHeap.handle;
}

id<MTLBuffer> MetalDevice::GetCbvSrvUavHeap() const
{
    return mCbvSrvUavHeap.handle;
}

id<MTLTextureViewPool> MetalDevice::GetRtvHeap() const
{
    return mCbvSrvUavHeap.pool;
}

id<MTLResidencySet> MetalDevice::GetResidencySet() const
{
    return mResidencySet;
}

id<MTL4ArgumentTable> MetalDevice::GetArgumentTable() const
{
    return mArgumentTable;
}

id<MTL4CommandQueue> MetalDevice::GetCommandQueue() const
{
    return mCommandQueue;
}

id<MTL4CommandBuffer> MetalDevice::AllocateCommandBuffer()
{
    auto swapchain = static_cast<MetalSwapchain*>(mSurface);
    auto& pool = mCommandPools[swapchain->mCurrentFrameIndex];
    
    id<MTL4CommandBuffer> commandBuffer = nil;
    if (pool.freeCommandBuffers.empty())
    {
        commandBuffer = [mHandle newCommandBuffer];
    }
    else
    {
        commandBuffer = (id<MTL4CommandBuffer>)CFBridgingRelease(pool.freeCommandBuffers.front());
        pool.freeCommandBuffers.pop_front();
    }
    pool.usedCommandBuffers.push_back((__bridge_retained void*)commandBuffer);
    [commandBuffer beginCommandBufferWithAllocator:pool.allocator];
    return commandBuffer;
}

IRRootSignature* MetalDevice::GetGlobalRootSignature() const
{
    return mRootSignature;
}

void MetalCommandPool::Reset()
{
    freeCommandBuffers.insert(freeCommandBuffers.end(), usedCommandBuffers.begin(), usedCommandBuffers.end());
    usedCommandBuffers.clear();
    [allocator reset];
}

void MetalCommandPool::Release()
{
    for (auto cmdBuffer : usedCommandBuffers)
    {
        CFRelease(cmdBuffer);
    }
    usedCommandBuffers.clear();

    for (auto cmdBuffer : freeCommandBuffers)
    {
        CFRelease(cmdBuffer);
    }
    freeCommandBuffers.clear();
    allocator = nil;
}

#endif
