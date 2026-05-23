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
#include <metal_irconverter_runtime/ir_raytracing.h>

using namespace Gleam;

static DeviceFeatures QueryDeviceFeatures(id<MTLDevice> device)
{
    DeviceFeatures features;
    features.raytracing = [device supportsRaytracing];
    features.meshShaders = [device supportsFamily:MTLGPUFamilyMetal3];
    features.rgb9e5UAVStores = [device supportsFamily:MTLGPUFamilyApple3] || [device supportsFamily:MTLGPUFamilyMac2];
    return features;
}

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

@interface MetalRayTracingPipelineImpl : NSObject<MetalRayTracingPipeline>
@property (nonatomic, strong) id<MTLComputePipelineState> pipelineState;
@property (nonatomic, strong) id<MTLIntersectionFunctionTable> intersectionFunctionTable;
@property (nonatomic, strong) id<MTLVisibleFunctionTable> visibleFunctionTable;
@end

@implementation MetalRayTracingPipelineImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _pipelineState = nil;
        _intersectionFunctionTable = nil;
        _visibleFunctionTable = nil;
    }
    return self;
}

@end

@interface MetalHitGroupImpl : NSObject
@property (nonatomic, strong) id<MetalRayTracingFunction> closestHit;
@property (nonatomic, strong) id<MetalRayTracingFunction> anyHit;
@property (nonatomic, strong) id<MetalRayTracingFunction> intersection;
@end

@implementation MetalHitGroupImpl

- (instancetype)init {
    self = [super init];
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
        handle = nil;
    }
    return self;
}

@synthesize handle;

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

@interface MetalRayTracingFunctionImpl : MetalFunctionImpl<MetalRayTracingFunction>
@property (nonatomic, assign) IRObject* dxil;
@end

@implementation MetalRayTracingFunctionImpl

- (instancetype)init {
    self = [super init];
    if (self) {
        _dxil = nil;
    }
    return self;
}

@end

void RenderSystem::InitializeBackend()
{
	mSwapchain = new MetalSwapchain();
    mReleaseQueue = new ResourceReleaseQueue(mSwapchain->GetFramesInFlight());
    mDevice = new MetalDevice(mSwapchain, mReleaseQueue);
}

static IRCompiler* CreateCompiler(const TString& entryPoint, IRRootSignature* globalRootSig)
{
    auto compiler = IRCompilerCreate();
    IRCompilerSetEntryPointName(compiler, entryPoint.c_str());
    IRCompilerSetMinimumDeploymentTarget(compiler, IROperatingSystem_macOS, "26.0");
    IRCompilerSetMinimumGPUFamily(compiler, IRGPUFamilyApple9);
    IRCompilerSetGlobalRootSignature(compiler, globalRootSig);
    return compiler;
}

static id<MetalFunction> CompileDXIL(id<MTLDevice> device, IRCompiler* compiler, IRObject* dxil, const TString& entryPoint, ShaderStage stage)
{
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
    id<MTLLibrary> library = [device newLibraryWithData:data error:&libraryError];
    if (libraryError)
    {
        auto errorStr = TO_CPP_STRING([libraryError localizedDescription]);
        GLEAM_CORE_ERROR("Metal library load failed: {0}", errorStr);
        libraryError = nil;
    }
    
    id<MTLFunction> mtlFunction = nil;
    NSString* entryPointStr = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    for (NSString* functionName in library.functionNames)
    {
        if ([functionName containsString:entryPointStr])
        {
            mtlFunction = [library newFunctionWithName:functionName];
            break;
        }
    }
    GLEAM_ASSERT(mtlFunction, "Metal: MTLFunction creation failed: {}", entryPoint);
    
    id<MetalFunction> function = nil;
    if (stage == ShaderStage::Compute)
    {
        MetalComputeFunctionImpl* computeFunction = [[MetalComputeFunctionImpl alloc] init];
        computeFunction.handle = mtlFunction;
        
        IRShaderReflection* reflection = IRShaderReflectionCreate();
        IRObjectGetReflection(metalIR, IRShaderStageCompute, reflection);
        
        IRVersionedCSInfo csInfo;
        IRShaderReflectionCopyComputeInfo(reflection, IRReflectionVersion_1_0, &csInfo);
        computeFunction.threadsPerThreadgroup = MTLSizeMake(csInfo.info_1_0.tg_size[0], csInfo.info_1_0.tg_size[1], csInfo.info_1_0.tg_size[2]);
        
        IRShaderReflectionReleaseComputeInfo(&csInfo);
        IRShaderReflectionDestroy(reflection);
        
        function = computeFunction;
    }
    else
    {
        MetalFunctionImpl* baseFunction = [[MetalFunctionImpl alloc] init];
        baseFunction.handle = mtlFunction;
        function = baseFunction;
    }
    
    // Clean up
    IRMetalLibBinaryDestroy(metallibBinary);
    IRObjectDestroy(metalIR);
    
    return function;
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
    desc.size = Math::AlignUp(sizeAndAlign.size, sizeAndAlign.align);

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
    texture.mHandle = static_cast<MetalDevice*>(this)->CreateTexture(allocator, descriptor);

    NSUInteger sliceCount = descriptor.depth;
    MTLTextureType mainViewType;
    switch (descriptor.dimension)
    {
        case TextureDimension::Texture2D:
        {
            mainViewType = descriptor.depth == 1 ? MTLTextureType2D : MTLTextureType2DArray;
            break;
        }
        case TextureDimension::Texture3D:
        {
            mainViewType = MTLTextureType3D;
            break;
        }
        case TextureDimension::TextureCube:
        {
            sliceCount = 6 * descriptor.depth;
            mainViewType = descriptor.depth == 1 ? MTLTextureTypeCube : MTLTextureTypeCubeArray;
            break;
        }
    }

    MTLTextureViewDescriptor* viewDesc = [MTLTextureViewDescriptor new];
    viewDesc.pixelFormat = TextureFormatToMTLPixelFormat(descriptor.format);
    viewDesc.textureType = mainViewType;
    viewDesc.levelRange = NSMakeRange(0, texture.mMipMapLevels);
    viewDesc.sliceRange = NSMakeRange(0, sliceCount);
    texture.mResourceView = static_cast<MetalDevice*>(this)->CreateResourceView(texture, viewDesc);

    texture.mSliceUnorderedAccessViews = static_cast<MetalDevice*>(this)->CreateUnorderedAccessViews(texture);
    if (descriptor.usage & TextureUsage_Attachment)
    {
        texture.mView = static_cast<MetalDevice*>(this)->CreateRenderTargetView(texture);
        texture.mSliceViews = static_cast<MetalDevice*>(this)->CreateRenderTargetViews(texture);
    }
    return texture;
}

id<MTLTexture> MetalDevice::CreateTexture(GPUAllocator* allocator, const TextureDescriptor& descriptor)
{
    MTLTextureDescriptor* textureDesc = [MTLTextureDescriptor new];
    textureDesc.pixelFormat = TextureFormatToMTLPixelFormat(descriptor.format);
    textureDesc.width = descriptor.size.width;
    textureDesc.height = descriptor.size.height;
    textureDesc.mipmapLevelCount = descriptor.useMipMap ? Texture::CalculateMipLevels(descriptor.size) : 1;
    textureDesc.arrayLength = 1;
    textureDesc.sampleCount = 1;
    textureDesc.depth = 1;
    textureDesc.usage = TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.storageMode = MTLStorageModePrivate;

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
    id<MTLTexture> texture = [heap newTextureWithDescriptor:textureDesc offset:allocation.offset];
    [texture setLabel:TO_NSSTRING(descriptor.name.c_str())];
    [mResidencySet addAllocation:texture];
    allocator->AddAllocation(texture, allocation);
    return texture;
}

RenderTargetView MetalDevice::CreateRenderTargetView(const Texture& texture)
{
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    auto entry = descriptorTable + texture.GetUnorderedAccessView().data;
    RenderTargetView rtv;
    rtv._impl = entry->textureViewID;
    return rtv;
}

TArray<RenderTargetView> MetalDevice::CreateRenderTargetViews(const Texture& texture)
{
    const auto& descriptor = texture.GetDescriptor();
    if (not (descriptor.usage & TextureUsage_Storage) || not Utils::IsColorFormat(descriptor.format))
    {
        return {};
    }

    NSUInteger sliceCount = descriptor.dimension == TextureDimension::TextureCube
        ? 6 * descriptor.depth : descriptor.depth;

    uint32_t mipMapLevels = texture.GetMipMapLevels();
    TArray<RenderTargetView> views;
    views.resize(sliceCount * mipMapLevels);

    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    for (uint32_t i = 0; i < views.size(); ++i)
    {
        uint32_t slice = Texture::GetSlice(i, mipMapLevels);
        uint32_t mip = Texture::GetMip(i, mipMapLevels);
        auto uav = texture.GetUnorderedAccessView(mip, slice);
        auto entry = descriptorTable + uav.data;
        views[i]._impl = entry->textureViewID;
    }
    return views;
}

TArray<ShaderResourceIndex> MetalDevice::CreateUnorderedAccessViews(const Texture& texture)
{
    const auto& descriptor = texture.GetDescriptor();
    if (not (descriptor.usage & TextureUsage_Storage) || not Utils::IsColorFormat(descriptor.format))
    {
        return {};
    }

    NSUInteger sliceCount = descriptor.dimension == TextureDimension::TextureCube
        ? 6 * descriptor.depth : descriptor.depth;

    uint32_t mipMapLevels = texture.GetMipMapLevels();
    TArray<ShaderResourceIndex> views;
    views.resize(sliceCount * mipMapLevels);

    for (uint32_t i = 0; i < views.size(); ++i)
    {
        uint32_t slice = Texture::GetSlice(i, mipMapLevels);
        uint32_t mip = Texture::GetMip(i, mipMapLevels);

        MTLTextureViewDescriptor* viewDesc = [MTLTextureViewDescriptor new];
        viewDesc.pixelFormat = TextureFormatToMTLPixelFormat(descriptor.format);
        switch (descriptor.dimension)
        {
            case TextureDimension::Texture2D:
            case TextureDimension::TextureCube:
            {
                viewDesc.textureType = MTLTextureType2D;
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
        views[i] = CreateResourceView(texture, viewDesc);
    }
    return views;
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

BottomLevelAccelerationStructure GraphicsDevice::CreateBLAS(const BLASDescriptor& descriptor)
{
	id<MTLAccelerationStructure> accelStructure = [mHandle newAccelerationStructureWithSize:descriptor.size];
	[accelStructure setLabel:TO_NSSTRING(descriptor.name.c_str())];
	[static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:accelStructure];
 
	return BottomLevelAccelerationStructure(descriptor, accelStructure);
}

TopLevelAccelerationStructure GraphicsDevice::CreateTLAS(const TLASDescriptor& descriptor)
{
	id<MTLAccelerationStructure> accelStructure = [mHandle newAccelerationStructureWithSize:descriptor.size];
	[accelStructure setLabel:TO_NSSTRING(descriptor.name.c_str())];
	[static_cast<MetalDevice*>(this)->GetResidencySet() addAllocation:accelStructure];

	TopLevelAccelerationStructure tlas(descriptor);
	tlas.mHandle = accelStructure;
	tlas.mResourceView = static_cast<MetalDevice*>(this)->CreateResourceView(tlas);
	return tlas;
}

Shader GraphicsDevice::CompileShader(const TString& entryPoint, ShaderStage stage)
{
    Shader shader(entryPoint, stage);
    auto shaderPath = Globals::BuiltinAssetsDirectory/"Shaders";
    auto shaderFile = Filesystem::OpenRead(shaderPath.Append(entryPoint + ".dxil"), FileType::Binary);
    auto shaderCode = shaderFile->Read();
    
    if (stage == ShaderStage::RayGeneration ||
        stage == ShaderStage::Miss ||
        stage == ShaderStage::ClosestHit ||
        stage == ShaderStage::AnyHit ||
        stage == ShaderStage::Intersection)
    {
        MetalRayTracingFunctionImpl* rayTracingFunction = [[MetalRayTracingFunctionImpl alloc] init];
        rayTracingFunction.dxil = IRObjectCreateFromDXIL((uint8_t*)shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipCopy);
        shader.mHandle = rayTracingFunction;
        return shader;
    }
    else
    {
        auto dxil = IRObjectCreateFromDXIL((uint8_t*)shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipNone);
        auto compiler = CreateCompiler(entryPoint, static_cast<MetalDevice*>(this)->GetGlobalRootSignature());
        shader.mHandle = CompileDXIL(mHandle, compiler, dxil, entryPoint, stage);
        IRCompilerDestroy(compiler);
        IRObjectDestroy(dxil);
        return shader;
    }
}

ComputePipeline GraphicsDevice::CompileComputePipeline(const ComputePipelineStateDescriptor& pipelineDesc)
{
    ComputePipeline pipeline(pipelineDesc);
    pipeline.mHandle = [[MetalComputePipelineImpl alloc] init];
    
	auto shader = CreateShader(pipelineDesc.entryPoint, ShaderStage::Compute);
    id<MetalComputeFunction> mtlFunction = shader.GetHandle();
    id<MetalComputePipeline> mtlPipeline = pipeline.mHandle;
    
    __autoreleasing NSError* error = nil;
    mtlPipeline.pipelineState = [mHandle newComputePipelineStateWithFunction:mtlFunction.handle error:&error];
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
    id<MetalFunction> vertexFunction = vertexShader.GetHandle();

    MTLRenderPipelineDescriptor* pipelineDescriptor = [MTLRenderPipelineDescriptor new];
    pipelineDescriptor.rasterSampleCount = 1;
    pipelineDescriptor.vertexFunction = vertexFunction.handle;
    if (not pipelineDesc.fragmentEntry.empty())
    {
        auto fragmentShader = CreateShader(pipelineDesc.fragmentEntry, ShaderStage::Fragment);
        id<MetalFunction> fragmentFunction = fragmentShader.GetHandle();
        pipelineDescriptor.fragmentFunction = fragmentFunction.handle;
    }
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

RayTracingPipeline GraphicsDevice::CompileRayTracingPipeline(const RayTracingPipelineStateDescriptor& pipelineDesc)
{
    RayTracingPipeline pipeline(pipelineDesc);
    pipeline.mHandle = [[MetalRayTracingPipelineImpl alloc] init];
    
    MTLComputePipelineDescriptor* mtlPipelineDesc = [[MTLComputePipelineDescriptor alloc] init];
    mtlPipelineDesc.maxCallStackDepth = pipelineDesc.maxRecursionDepth;
    
    auto rayGenShader = CreateShader(pipelineDesc.rayGenerationEntry, ShaderStage::RayGeneration);
    MetalDevice* device = static_cast<MetalDevice*>(this);
    id<MetalRayTracingPipeline> mtlPipeline = pipeline.mHandle;
    id<MetalRayTracingFunction> rayGenFunction = rayGenShader.GetHandle();

    TArray<TString> missEntryNames;
    NSMutableArray<id<MetalRayTracingFunction>>* missFunctions = [NSMutableArray array];
    uint64_t missMask = 0;
    for (const auto& missEntry : pipelineDesc.missEntries)
    {
        if (not missEntry.empty())
        {
            auto shader = CreateShader(missEntry, ShaderStage::Miss);
            id<MetalRayTracingFunction> fn = shader.GetHandle();
            missMask |= IRObjectGatherRaytracingIntrinsics(fn.dxil, missEntry.c_str());
            missEntryNames.emplace_back(missEntry);
            [missFunctions addObject:fn];
        }
    }

    uint64_t closestHitMask = 0;
    uint64_t anyHitMask = 0;

    NSMutableArray<MetalHitGroupImpl*>* hitGroups = [NSMutableArray arrayWithCapacity:pipelineDesc.hitGroups.size()];
    for (const auto& hitGroup : pipelineDesc.hitGroups)
    {
        MetalHitGroupImpl* mtlHitGroup = [[MetalHitGroupImpl alloc] init];
        if (not hitGroup.closestHitEntry.empty())
        {
            auto shader = CreateShader(hitGroup.closestHitEntry, ShaderStage::ClosestHit);
            mtlHitGroup.closestHit = shader.GetHandle();
            closestHitMask |= IRObjectGatherRaytracingIntrinsics(mtlHitGroup.closestHit.dxil, hitGroup.closestHitEntry.c_str());
        }
        if (not hitGroup.anyHitEntry.empty())
        {
            auto shader = CreateShader(hitGroup.anyHitEntry, ShaderStage::AnyHit);
            mtlHitGroup.anyHit = shader.GetHandle();
            anyHitMask |= IRObjectGatherRaytracingIntrinsics(mtlHitGroup.anyHit.dxil, hitGroup.anyHitEntry.c_str());
        }
        if (not hitGroup.intersectionEntry.empty())
        {
            auto shader = CreateShader(hitGroup.intersectionEntry, ShaderStage::Intersection);
            mtlHitGroup.intersection = shader.GetHandle();
        }
        [hitGroups addObject:mtlHitGroup];
    }
    
    IRRayTracingPipelineConfiguration* rtConfig = IRRayTracingPipelineConfigurationCreate();
    IRRayTracingPipelineConfigurationSetRayGenerationCompilationMode(rtConfig, IRRayGenerationCompilationKernel);
    IRRayTracingPipelineConfigurationSetIntersectionFunctionCompilationMode(rtConfig, IRIntersectionFunctionCompilationIntersectionFunctionBufferFunction);
    IRRayTracingPipelineConfigurationSetMaxAttributeSizeInBytes(rtConfig, pipelineDesc.maxAttributeSize);
    IRRayTracingPipelineConfigurationSetMaxRecursiveDepth(rtConfig, pipelineDesc.maxRecursionDepth);
    IRRayTracingPipelineConfigurationSetIntrinsicMasks(rtConfig, closestHitMask, missMask, anyHitMask, /*callable=*/0);
    
    // RayGeneration
    {
        auto compiler = CreateCompiler(pipelineDesc.rayGenerationEntry, device->GetGlobalRootSignature());
        IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
        auto function = CompileDXIL(device->GetHandle(), compiler, rayGenFunction.dxil, pipelineDesc.rayGenerationEntry, ShaderStage::RayGeneration);
        IRCompilerDestroy(compiler);
        
        mtlPipelineDesc.computeFunction = function.handle;
    }
    
    NSMutableArray<id<MTLFunction>>* missGroup       = [NSMutableArray array];
    NSMutableArray<id<MTLFunction>>* closestHitGroup = [NSMutableArray array];
    
    // Miss
    for (NSUInteger i = 0; i < missFunctions.count; ++i)
    {
        id<MetalRayTracingFunction> fn = missFunctions[i];
        auto compiler = CreateCompiler(missEntryNames[i], device->GetGlobalRootSignature());
        IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
        auto function = CompileDXIL(device->GetHandle(), compiler, fn.dxil, missEntryNames[i], ShaderStage::Miss);
        IRCompilerDestroy(compiler);
        [missGroup addObject:function.handle];
    }

    // Intersection function buffer functions (custom intersection / any-hit)
    NSMutableArray<id<MTLFunction>>* intersectionFunctions = [NSMutableArray array];

    // Per-hit-group IFT slot: NSNotFound means opaque triangle (no custom intersection)
    TArray<NSUInteger> intersectionFunctionSlots(pipelineDesc.hitGroups.size(), NSNotFound);

    for (NSUInteger i = 0; i < hitGroups.count; ++i)
    {
        MetalHitGroupImpl* hitGroup = hitGroups[i];
        const auto& hitGroupDesc = pipelineDesc.hitGroups[i];
        
        if (hitGroup.closestHit)
        {
            auto compiler = CreateCompiler(hitGroupDesc.closestHitEntry, device->GetGlobalRootSignature());
            IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
            IRCompilerSetHitgroupType(compiler, IRHitGroupTypeTriangles);
            auto function = CompileDXIL(device->GetHandle(), compiler, hitGroup.closestHit.dxil, hitGroupDesc.closestHitEntry, ShaderStage::ClosestHit);
            IRCompilerDestroy(compiler);
            
            [closestHitGroup addObject:function.handle];
        }
        
        if (hitGroup.intersection && hitGroup.anyHit)
        {
            IRCompiler* combineCompiler = IRCompilerCreate();
            IRCompilerSetRayTracingPipelineConfiguration(combineCompiler, rtConfig);
            IRCompilerSetHitgroupType(combineCompiler, IRHitGroupTypeProceduralPrimitive);

            IRError* combineError = nullptr;
            IRObject* combinedDxil = IRCompilerAllocCombineCompileAndLink(combineCompiler,
                                                                          hitGroupDesc.intersectionEntry.c_str(),
                                                                          hitGroup.intersection.dxil,
                                                                          hitGroupDesc.anyHitEntry.c_str(),
                                                                          hitGroup.anyHit.dxil,
                                                                          &combineError);
            if (combineError)
            {
                GLEAM_CORE_ERROR("Metal: Failed to compile IFB function '{}': {}", hitGroupDesc.name, IRErrorGetCode(combineError));
                IRErrorDestroy(combineError);
            }
            IRCompilerDestroy(combineCompiler);

            auto compiler = CreateCompiler(hitGroupDesc.intersectionEntry, device->GetGlobalRootSignature());
            IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
            IRCompilerSetHitgroupType(compiler, IRHitGroupTypeProceduralPrimitive);
            auto function = CompileDXIL(device->GetHandle(),
                                        compiler,
                                        combinedDxil,
                                        hitGroupDesc.intersectionEntry,
                                        ShaderStage::Intersection);
            IRCompilerDestroy(compiler);
            IRObjectDestroy(combinedDxil);

            intersectionFunctionSlots[i] = (NSUInteger)[intersectionFunctions count];
            [intersectionFunctions addObject:function.handle];
        }
        else if (hitGroup.anyHit)
        {
            auto compiler = CreateCompiler(hitGroupDesc.anyHitEntry, device->GetGlobalRootSignature());
            IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
            IRCompilerSetHitgroupType(compiler, IRHitGroupTypeTriangles);
            auto function = CompileDXIL(device->GetHandle(), compiler, hitGroup.anyHit.dxil, hitGroupDesc.anyHitEntry, ShaderStage::AnyHit);
            IRCompilerDestroy(compiler);
            
            [intersectionFunctions addObject:function.handle];
        }
        else if (hitGroup.intersection)
        {
            auto compiler = CreateCompiler(hitGroupDesc.intersectionEntry, device->GetGlobalRootSignature());
            IRCompilerSetRayTracingPipelineConfiguration(compiler, rtConfig);
            IRCompilerSetHitgroupType(compiler, IRHitGroupTypeProceduralPrimitive);
            auto function = CompileDXIL(device->GetHandle(), compiler, hitGroup.intersection.dxil, hitGroupDesc.intersectionEntry, ShaderStage::Intersection);
            IRCompilerDestroy(compiler);
            
            [intersectionFunctions addObject:function.handle];
        }
    }
    IRRayTracingPipelineConfigurationDestroy(rtConfig);
    
    MTLLinkedFunctions* linkedFunctions = [[MTLLinkedFunctions alloc] init];
    NSMutableArray<id<MTLFunction>>* allVisible = [NSMutableArray array];
    [allVisible addObjectsFromArray:missGroup];
    [allVisible addObjectsFromArray:closestHitGroup];
    [allVisible addObjectsFromArray:intersectionFunctions];
    linkedFunctions.functions = allVisible;
    linkedFunctions.groups = @{
        @(kIRFunctionGroupMiss)       : missGroup,
        @(kIRFunctionGroupClosestHit) : closestHitGroup,
    };
    mtlPipelineDesc.linkedFunctions = linkedFunctions;

    __autoreleasing NSError* error = nil;
    mtlPipeline.pipelineState = [mHandle newComputePipelineStateWithDescriptor:mtlPipelineDesc
                                                                       options:MTLPipelineOptionNone
                                                                    reflection:nil
                                                                         error:&error];
    GLEAM_ASSERT(mtlPipeline.pipelineState, "Metal: Ray tracing pipeline state creation failed.");

    if (pipelineDesc.hitGroups.size() > 0)
    {
        MTLIntersectionFunctionTableDescriptor* iftDesc = [[MTLIntersectionFunctionTableDescriptor alloc] init];
        iftDesc.functionCount = (NSUInteger)pipelineDesc.hitGroups.size();
 
        mtlPipeline.intersectionFunctionTable = [mtlPipeline.pipelineState newIntersectionFunctionTableWithDescriptor:iftDesc];
        GLEAM_ASSERT(mtlPipeline.intersectionFunctionTable, "Metal: Intersection function table creation failed.");
 
        for (uint32_t i = 0; i < (uint32_t)pipelineDesc.hitGroups.size(); ++i)
        {
            if (pipelineDesc.hitGroups[i].name.empty())
            {
                continue;
            }
            NSUInteger slot = intersectionFunctionSlots[i];
            if (slot != NSNotFound)
            {
                id<MTLFunctionHandle> handle =
                    [mtlPipeline.pipelineState functionHandleWithFunction:intersectionFunctions[slot]];
                [mtlPipeline.intersectionFunctionTable setFunction:handle atIndex:(NSUInteger)i];
            }
            else
            {
                // Opaque triangle geometry: use the built-in triangle intersection
                [mtlPipeline.intersectionFunctionTable setOpaqueTriangleIntersectionFunctionWithSignature:MTLIntersectionFunctionSignatureInstancing
                                                                                                  atIndex:(NSUInteger)i];
            }
        }
    }

    // Create visible function table
    {
        // VFT layout (index 0 is reserved as null):
        //   1              : miss shader
        //   2 .. 1+numCH   : closest-hit shaders (in hit group order, only those that exist)
        NSUInteger vftSize = 1 + [missGroup count] + [closestHitGroup count];
        MTLVisibleFunctionTableDescriptor* vftDesc = [[MTLVisibleFunctionTableDescriptor alloc] init];
        vftDesc.functionCount = vftSize;

        mtlPipeline.visibleFunctionTable = [mtlPipeline.pipelineState newVisibleFunctionTableWithDescriptor:vftDesc];
        GLEAM_ASSERT(mtlPipeline.visibleFunctionTable, "Metal: Visible function table creation failed.");

        NSUInteger vftIndex = 1;
        for (id<MTLFunction> f in missGroup)
        {
            id<MTLFunctionHandle> handle = [mtlPipeline.pipelineState functionHandleWithFunction:f];
            [mtlPipeline.visibleFunctionTable setFunction:handle atIndex:vftIndex++];
        }

        for (id<MTLFunction> f in closestHitGroup)
        {
            id<MTLFunctionHandle> handle = [mtlPipeline.pipelineState functionHandleWithFunction:f];
            [mtlPipeline.visibleFunctionTable setFunction:handle atIndex:vftIndex++];
        }
    }
    
    pipeline.mShaderBindingTable = static_cast<MetalDevice*>(this)->CreateShaderBindingTable(pipeline);
    return pipeline;
}

void GraphicsDevice::Dispose(Heap& heap)
{
    mReleaseQueue->AddResource([this, resource = heap.GetHandle()]()
    {
        [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
    }, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
    heap.mHandle = nil;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Buffer& buffer, BarrierStage stage)
{
    if (buffer.GetDescriptor().memoryType == MemoryType::CPU)
    {
        mReleaseQueue->AddResource([this,
                                    allocator = allocator,
                                    resource = buffer.GetHandle(),
                                    view = buffer.GetResourceView()]()
        {
            const auto& allocation = allocator->GetAllocation(resource);
            allocator->Free(allocation, BarrierStage::None);

            [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
            static_cast<MetalDevice*>(this)->ReleaseResourceView(view);
        }, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
    }
    else // if (buffer.GetDescriptor().memoryType == MemoryType::GPU)
    {
        const auto& allocation = allocator->GetAllocation(buffer.GetHandle());
        allocator->Free(allocation, stage);

        mReleaseQueue->AddResource([this,
                                    resource = buffer.GetHandle(),
                                    view = buffer.GetResourceView()]()
        {
            [static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
            static_cast<MetalDevice*>(this)->ReleaseResourceView(view);
        }, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
    }
    
    
	buffer.mResourceView = InvalidResourceIndex;
	buffer.mContents = nullptr;
	buffer.mHandle = nil;
}

void GraphicsDevice::Dispose(GPUAllocator* allocator, Texture& texture, BarrierStage stage)
{
    const auto& allocation = allocator->GetAllocation(texture.GetHandle());
	allocator->Free(allocation, stage);
    
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

void GraphicsDevice::Dispose( BottomLevelAccelerationStructure& blas)
{
	mReleaseQueue->AddResource([this, resource = blas.GetHandle()]()
	{
		[static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
	blas.mHandle = nil;
}
 
void GraphicsDevice::Dispose(TopLevelAccelerationStructure& tlas)
{
	mReleaseQueue->AddResource([this,
								resource = tlas.GetHandle(),
								view = tlas.GetResourceView()]()
	{
		[static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:resource];
		static_cast<MetalDevice*>(this)->ReleaseResourceView(view);
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());
 
	tlas.mResourceView = {};
	tlas.mHandle = nil;
}

void GraphicsDevice::Dispose(Shader& shader)
{
    eastl::erase_if(mShaderCache, [&shader](const auto& cache) -> bool
	{
		return cache.GetHandle() == shader.GetHandle();
	});

    if (shader.mStage == ShaderStage::RayGeneration ||
        shader.mStage == ShaderStage::Miss ||
        shader.mStage == ShaderStage::ClosestHit ||
        shader.mStage == ShaderStage::AnyHit ||
        shader.mStage == ShaderStage::Intersection)
    {
        id<MetalRayTracingFunction> function = shader.mHandle;
        IRObjectDestroy(function.dxil);
    }
	shader.mHandle = nil;
}

void GraphicsDevice::Dispose(ComputePipeline& pipeline)
{
    mComputePipelineCache.erase(ComputePipelineHandle(pipeline.GetHash()));
    pipeline.mHandle = nil;
}

void GraphicsDevice::Dispose(GraphicsPipeline& pipeline)
{
    mGraphicsPipelineCache.erase(GraphicsPipelineHandle(pipeline.GetHash()));
    pipeline.mHandle = nil;
}

void GraphicsDevice::Dispose(RayTracingPipeline& pipeline)
{
    mRayTracingPipelineCache.erase(RayTracingPipelineHandle(pipeline.GetHash()));
    mReleaseQueue->AddResource([this,
								sbt = pipeline.GetShaderBindingTable().GetHandle()]()
	{
		[static_cast<MetalDevice*>(this)->GetResidencySet() removeAllocation:sbt];
	}, static_cast<Swapchain*>(mSurface)->GetFrameIndex());

    pipeline.mHandle = nil;
    pipeline.mShaderBindingTable = {};
}

MetalDevice::MetalDevice(RenderSurface* surface, ResourceReleaseQueue* releaseQueue)
    : GraphicsDevice(surface, releaseQueue)
{
    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);

    mFeatures = QueryDeviceFeatures(mHandle);

    // init MTLResidencySet
    __autoreleasing NSError* residencySetError = nil;
    MTLResidencySetDescriptor* residencySetDesc = [MTLResidencySetDescriptor new];
    residencySetDesc.initialCapacity = CBV_SRV_HEAP_SIZE;
    residencySetDesc.label = @"ResidencySet";
    mResidencySet = [mHandle newResidencySetWithDescriptor:residencySetDesc error:&residencySetError];
    GLEAM_ASSERT(mResidencySet, "Metal: Residency set creation failed.");
    
    // init MTL4CommandQueue
    mCommandQueue.mDevice = mHandle;
    mCommandQueue.mHandle = [mHandle newMTL4CommandQueue];
    mCommandQueue.mFence = [mHandle newSharedEvent];
    [mCommandQueue.mHandle addResidencySet:mResidencySet];
    
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
    constexpr uint32_t NumRootParams = PUSH_CONSTANT_SLOT + 2; // 1 for push constants, 1 for samplers descriptor table
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
    
    // Static samplers
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
                                                        | IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed
                                                        | IRRootSignatureFlagSamplerHeapDirectlyIndexed);

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
    argumentTableDesc.initializeBindings = false;
    argumentTableDesc.supportAttributeStrides = true;
    argumentTableDesc.maxSamplerStateBindCount = 0;
    argumentTableDesc.maxTextureBindCount = 0;
    argumentTableDesc.maxBufferBindCount = 15;
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
    mRayTracingPipelineCache.clear();
    IRRootSignatureDestroy(mRootSignature);
    
    // Destroy descriptor heap
    for (void* sampler : mStaticSamplers)
    {
        CFRelease(sampler);
    }
    mStaticSamplers.clear();
    mSamplerHeap.handle = nil;
#ifdef USE_TEXTURE_VIEW_POOL
    mCbvSrvUavHeap.pool = nil;
#else
    mCbvSrvUavHeap.textureViews = nil;
#endif
    mCbvSrvUavHeap.handle = nil;
    
    // Destroy residency set
    [mCommandQueue.GetHandle() removeResidencySet:mResidencySet];
    mResidencySet = nil;
    mArgumentTable = nil;

    // Destroy command queue
    mCommandQueue.Release();

    // Destroy device
    mHandle = nil;

    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

void MetalDevice::Configure(const RendererConfig& config)
{
    auto swapchain = static_cast<MetalSwapchain*>(mSurface);
    swapchain->Configure(this, config);
}

ShaderBindingTable MetalDevice::CreateShaderBindingTable(const RayTracingPipeline& pipeline)
{
    constexpr uint32_t shaderRecordSize = sizeof(IRShaderIdentifier);
    constexpr uint32_t rayGenTableSize = shaderRecordSize;

    const auto& pipelineDesc = pipeline.GetDescriptor();
    uint32_t missTableSize = static_cast<uint32_t>(pipelineDesc.missEntries.size()) * shaderRecordSize;
    uint32_t hitGroupTableSize = static_cast<uint32_t>(pipelineDesc.hitGroups.size()) * shaderRecordSize;
    uint32_t totalSize = rayGenTableSize + missTableSize + hitGroupTableSize;

    id<MTLBuffer> sbtBuffer = [mHandle newBufferWithLength:totalSize options:MTLResourceStorageModeShared];
    [sbtBuffer setLabel:@"ShaderBindingTable"];
    [mResidencySet addAllocation:sbtBuffer];

    void* sbtPtr = [sbtBuffer contents];
    uint32_t offset = 0;

    // Ray generation record
    // In kernel compilation mode, ray gen is the compute function itself.
    // The SBT record exists for IRDispatchRaysDescriptor compatibility;
    // shaderHandle 0 denotes "no VFT lookup needed".
    {
        IRShaderIdentifier* ident = static_cast<IRShaderIdentifier*>(OffsetPointer(sbtPtr, offset));
        IRShaderIdentifierInit(ident, 0);
    }
    offset += rayGenTableSize;

    // Miss records — VFT indices starting at 1
    for (uint32_t i = 0; i < static_cast<uint32_t>(pipelineDesc.missEntries.size()); ++i)
    {
        if (not pipelineDesc.missEntries[i].empty())
        {
            IRShaderIdentifier* ident = static_cast<IRShaderIdentifier*>(OffsetPointer(sbtPtr, offset));
            IRShaderIdentifierInit(ident, 1 + i);
        }
        offset += shaderRecordSize;
    }

    // Hit group records
    // VFT layout: index 1..N = miss shaders, index N+1+ = closest-hit in order of hit groups that have one.
    // closestHitVFTIndex tracks the next available VFT slot.
    {
        uint64_t closestHitVFTIndex = 1 + static_cast<uint64_t>(pipelineDesc.missEntries.size()); // starts after reserved(0) + N miss shaders

        for (uint32_t i = 0; i < pipelineDesc.hitGroups.size(); i++)
        {
            IRShaderIdentifier* ident = static_cast<IRShaderIdentifier*>(OffsetPointer(sbtPtr, offset));
            const auto& hitGroup = pipelineDesc.hitGroups[i];

            if (not hitGroup.closestHitEntry.empty())
            {
                if (not hitGroup.intersectionEntry.empty())
                {
                    // Custom intersection is in the MTLIntersectionFunctionTable (IFB mode),
                    // not the VFT. The IFT index is resolved by Metal during traversal via
                    // geometry+instance intersection function offsets. Pass 0 for
                    // intersectionShaderHandle since the IFT handles dispatch directly.
                    IRShaderIdentifierInitWithCustomIntersection(ident, closestHitVFTIndex, 0);
                }
                else
                {
                    IRShaderIdentifierInit(ident, closestHitVFTIndex);
                }
                closestHitVFTIndex++;
            }
            else
            {
                // No closest-hit shader — null handle
                IRShaderIdentifierInit(ident, 0);
            }

            offset += shaderRecordSize;
        }
    }

    uint64_t baseAddress = [sbtBuffer gpuAddress];
	GPUVirtualAddressRange rayGenRecord = { .startAddress = baseAddress, .sizeInBytes = rayGenTableSize };
	GPUVirtualAddressRangeAndStride missRecord = { .startAddress = baseAddress + rayGenTableSize, .sizeInBytes = missTableSize, .strideInBytes = shaderRecordSize };
	GPUVirtualAddressRangeAndStride hitGroupRecord = { .startAddress = baseAddress + rayGenTableSize + missTableSize, .sizeInBytes = hitGroupTableSize, .strideInBytes = shaderRecordSize };
	return ShaderBindingTable(sbtBuffer, rayGenRecord, missRecord, hitGroupRecord);
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
    auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
    auto entry = descriptorTable + index.data;

#ifdef USE_TEXTURE_VIEW_POOL
    auto resourceID = [mCbvSrvUavHeap.pool setTextureView:texture.GetHandle() descriptor:viewDesc atIndex:index.data];
    entry->gpuVA = 0;
    entry->textureViewID = resourceID._impl;
    entry->metadata = 0;
#else
    id<MTLTexture> typedView = [texture.GetHandle()
        newTextureViewWithPixelFormat:viewDesc.pixelFormat
                         textureType:viewDesc.textureType
                              levels:viewDesc.levelRange
                              slices:viewDesc.sliceRange];
    mCbvSrvUavHeap.textureViews[@(index.data)] = typedView;
    [mResidencySet addAllocation:typedView];
    IRDescriptorTableSetTexture(entry, typedView, 0.0f, 0);
#endif

    return index;
}

AccelerationStructureView MetalDevice::CreateResourceView(const TopLevelAccelerationStructure& tlas)
{
    id<MTLBuffer> headerBuffer = [mHandle newBufferWithLength:sizeof(IRRaytracingAccelerationStructureGPUHeader)
                                                            + sizeof(uint32_t) * tlas.GetDescriptor().instanceCount
                                                      options:MTLResourceStorageModeShared];
	[headerBuffer setLabel:@"TLAS GPU Header"];
	[mResidencySet addAllocation:headerBuffer];

    auto index = mCbvSrvUavHeap.heap.Allocate();
	auto descriptorTable = static_cast<IRDescriptorTableEntry*>([mCbvSrvUavHeap.handle contents]);
	IRDescriptorTableSetAccelerationStructure(descriptorTable + index.data, [headerBuffer gpuAddress]);
 
	AccelerationStructureView view;
	view.index = index;
	view.header = headerBuffer;
	return view;
}

void MetalDevice::ReleaseResourceView(ShaderResourceIndex view)
{
    if (view != InvalidResourceIndex)
    {
#ifndef USE_TEXTURE_VIEW_POOL
        NSNumber* key = @(view.data);
        id<MTLTexture> typedView = mCbvSrvUavHeap.textureViews[key];
        if (typedView)
        {
            [mResidencySet removeAllocation:typedView];
            [mCbvSrvUavHeap.textureViews removeObjectForKey:key];
        }
#endif
        mCbvSrvUavHeap.heap.Release(view);
    }
}

void MetalDevice::ReleaseResourceView(AccelerationStructureView view)
{
    if (view.index != InvalidResourceIndex)
    {
        [mResidencySet removeAllocation:view.header];
        mCbvSrvUavHeap.heap.Release(view.index);
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
    [mResidencySet addAllocation:heap.handle];
    return heap;
}

MetalDescriptorHeap MetalDevice::CreateDescriptorHeap(uint32_t capacity) const
{
    MetalDescriptorHeap heap;
    heap.handle = [mHandle newBufferWithLength:capacity * sizeof(IRDescriptorTableEntry) options:MTLResourceStorageModeShared];
    heap.heap = ResourceDescriptorHeap(capacity);
    [heap.handle setLabel:@"DescriptorHeap"];
    [mResidencySet addAllocation:heap.handle];

#ifdef USE_TEXTURE_VIEW_POOL
    __autoreleasing NSError* error = nil;
    MTLResourceViewPoolDescriptor* desc = [MTLResourceViewPoolDescriptor new];
    desc.resourceViewCount = capacity;
    desc.label = @"TextureViewPool";
    heap.pool = [mHandle newTextureViewPoolWithDescriptor:desc error:&error];
#else
    heap.textureViews = [NSMutableDictionary dictionary];
#endif

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

#ifdef USE_TEXTURE_VIEW_POOL
id<MTLTextureViewPool> MetalDevice::GetRtvHeap() const
{
    return mCbvSrvUavHeap.pool;
}
#endif

id<MTLResidencySet> MetalDevice::GetResidencySet() const
{
    return mResidencySet;
}

id<MTL4ArgumentTable> MetalDevice::GetArgumentTable() const
{
    return mArgumentTable;
}

MetalCommandQueue& MetalDevice::GetCommandQueue()
{
    return mCommandQueue;
}

IRRootSignature* MetalDevice::GetGlobalRootSignature() const
{
    return mRootSignature;
}

#endif
