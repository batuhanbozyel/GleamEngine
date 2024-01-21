#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "MetalSwapchain.h"
#include "MetalShaderReflect.h"
#include "MetalPipelineStateManager.h"

#include "Core/Application.h"

using namespace Gleam;

Scope<GraphicsDevice> GraphicsDevice::Create()
{
    return CreateScope<MetalDevice>();
}

void GraphicsDevice::Configure(const RendererConfig& config)
{
    As<MetalSwapchain*>(mSwapchain.get())->Configure(config);
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

Heap GraphicsDevice::AllocateHeap(const HeapDescriptor& descriptor) const
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
    return heap;
}

Texture GraphicsDevice::AllocateTexture(const TextureDescriptor& descriptor) const
{
    Texture texture(descriptor);
    
    MTLTextureDescriptor* textureDesc;
    if (descriptor.type == TextureType::TextureCube)
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
    textureDesc.usage = descriptor.type == TextureType::RenderTexture ? TextureUsageToMTLTextureUsage(descriptor.usage) | MTLTextureUsageRenderTarget : TextureUsageToMTLTextureUsage(descriptor.usage);
    textureDesc.storageMode = MTLStorageModePrivate;
    texture.mHandle = [mHandle newTextureWithDescriptor:textureDesc];
    
    id<MTLTexture> baseTexture = texture.mHandle;
    texture.mView = [baseTexture newTextureViewWithPixelFormat:baseTexture.pixelFormat
                                           textureType:descriptor.type == TextureType::TextureCube ? MTLTextureTypeCubeArray : MTLTextureType2DArray
                                                levels:NSMakeRange(0, texture.mMipMapLevels)
                                                slices:NSMakeRange(0, 1)];
    
    if (descriptor.sampleCount > 1)
    {
        MTLTextureDescriptor* msaaTextureDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:baseTexture.pixelFormat width:descriptor.size.width height:descriptor.size.height mipmapped:false];
        msaaTextureDesc.textureType = MTLTextureType2DMultisample;
        msaaTextureDesc.mipmapLevelCount = 1;
        msaaTextureDesc.sampleCount = descriptor.sampleCount;
        msaaTextureDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderWrite;
        msaaTextureDesc.storageMode = MTLStorageModePrivate; // TODO: Switch to memoryless msaa render targets when Tile shading is supported
        texture.mMultisampleHandle = [mHandle newTextureWithDescriptor:msaaTextureDesc];
        texture.mMultisampleView = texture.mMultisampleHandle;
    }
    return texture;
}

Shader GraphicsDevice::GenerateShader(const TString& entryPoint, ShaderStage stage) const
{
    Shader shader(entryPoint, stage);
    
    TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GameInstance->GetDefaultAssetPath().append(entryPoint + ".dxil"));
    
    auto compiler = IRCompilerCreate();
    IRCompilerSetEntryPointName(compiler, entryPoint.data());
    
    if (stage == ShaderStage::Vertex)
    {
        IRCompilerSetStageInGenerationMode(compiler, IRStageInCodeGenerationModeUseSeparateStageInFunction);
    }
    
    auto dxil = IRObjectCreateFromDXIL(shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipNone);
    auto metalIR = IRCompilerAllocCompileAndLink(compiler, nullptr, 0, dxil, nullptr);
    
    IRMetalLibBinary* metallibBinary = IRMetalLibBinaryCreate();
    IRObjectGetMetalLibBinary(metalIR, IRObjectGetMetalIRShaderStage(metalIR), metallibBinary);
    dispatch_data_t data = IRMetalLibGetBytecodeData(metallibBinary);
    
    NSError* __autoreleasing error = nil;
    id<MTLLibrary> library = [mHandle newLibraryWithData:data error:&error];
    
    NSString* functionName = [NSString stringWithCString:entryPoint.c_str() encoding:NSASCIIStringEncoding];
    shader.mHandle = [library newFunctionWithName:functionName];
    shader.mReflection = CreateRef<Shader::Reflection>(metalIR);
    
    // Clean up
    IRMetalLibBinaryDestroy(metallibBinary);
    IRObjectDestroy(dxil);
    IRObjectDestroy(metalIR);
    IRCompilerDestroy(compiler);
    
    return shader;
}

void GraphicsDevice::Dispose(Heap& heap) const
{
    heap.mHandle = nil;
}

void GraphicsDevice::Dispose(Buffer& buffer) const
{
    buffer.mHandle = nil;
}

void GraphicsDevice::Dispose(Texture& texture) const
{
    texture.mHandle = nil;
    texture.mView = nil;
    
    if (texture.GetDescriptor().sampleCount > 1)
    {
        texture.mMultisampleHandle = nil;
        texture.mMultisampleView = nil;
    }
}

MetalDevice::MetalDevice()
{
    mSwapchain = CreateScope<MetalSwapchain>();

    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);

    // init CAMetalLayer
    As<MetalSwapchain*>(mSwapchain.get())->Initialize(this);

    // init MTLCommandQueue
    mCommandPool = [mHandle newCommandQueue];

    MetalPipelineStateManager::Init(this);

    GLEAM_CORE_INFO("Metal: Graphics device created.");
}

MetalDevice::~MetalDevice()
{
    // Destroy swapchain
    As<MetalSwapchain*>(mSwapchain.get())->Destroy();

    mShaderCache.clear();
    Clear();

    MetalPipelineStateManager::Destroy();

    // Destroy command pool
    mCommandPool = nil;

    // Destroy device
    mHandle = nil;

    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

id<MTLDevice> MetalDevice::GetHandle() const
{
    return mHandle;
}

id<MTLCommandQueue> MetalDevice::GetCommandPool() const
{
    return mCommandPool;
}

#endif
