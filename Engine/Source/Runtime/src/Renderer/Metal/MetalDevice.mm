#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "MetalShaderReflect.h"
#include "MetalPipelineStateManager.h"

#include "Core/WindowSystem.h"
#include "Core/Application.h"
#include "Core/Events/RendererEvent.h"

using namespace Gleam;

Scope<GraphicsDevice> GraphicsDevice::Create()
{
    return CreateScope<MetalDevice>();
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
    
    TArray<uint8_t> shaderCode = IOUtils::ReadBinaryFile(GameInstance->GetDefaultAssetPath().append("Shaders/" + entryPoint + ".dxil"));
    auto dxil = IRObjectCreateFromDXIL(shaderCode.data(), shaderCode.size(), IRBytecodeOwnershipNone);
    
    auto compiler = IRCompilerCreate();
    IRCompilerSetEntryPointName(compiler, entryPoint.data());
    IRCompilerSetGlobalRootSignature(compiler, MetalPipelineStateManager::GetGlobalRootSignature());
    
    IRError* compileError = nullptr;
    auto metalIR = IRCompilerAllocCompileAndLink(compiler, entryPoint.c_str(), dxil, &compileError);
    if (compileError)
    {
        auto errorCode = IRErrorGetCode(compileError);
        GLEAM_ERROR("Metal IR generation failed with code: {0}", errorCode);
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
        GLEAM_ERROR("Metal library load failed: {0}", errorStr);
        libraryError = nil;
    }
    
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
    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);

    // init CAMetalLayer
    auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();
    
    // Create surface
    mSurface = SDL_Metal_CreateView(windowSystem->GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    mSwapchain = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    mSwapchain.name = [NSString stringWithCString:windowSystem->GetConfiguration().title.c_str() encoding:NSASCIIStringEncoding];
    mSwapchain.device = mHandle;
    mSwapchain.framebufferOnly = NO;
    mSwapchain.opaque = YES;
    
    const auto& resolution = windowSystem->GetResolution();
    mSize = resolution * mSwapchain.contentsScale;
    mSwapchain.drawableSize = CGSizeMake(mSize.width, mSize.height);
    mFormat = MTLPixelFormatToTextureFormat(mSwapchain.pixelFormat);
    
    EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
    {
        mSize.width = e.GetWidth() * mSwapchain.contentsScale;
        mSize.height = e.GetHeight() * mSwapchain.contentsScale;
        mSwapchain.drawableSize = CGSizeMake(mSize.width, mSize.height);
        EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(mSize));
    });
    
    mImageAcquireSemaphore = dispatch_semaphore_create(mMaxFramesInFlight);

    // init MTLCommandQueue
    mCommandPool = [mHandle newCommandQueue];

    MetalPipelineStateManager::Init(this);

    GLEAM_CORE_INFO("Metal: Graphics device created.");
}

MetalDevice::~MetalDevice()
{
    // Destroy swapchain
    mImageAcquireSemaphore = nil;
    mDrawable = nil;
    mHandle = nil;
    mSurface = nil;

    mShaderCache.clear();

    MetalPipelineStateManager::Destroy();

    // Destroy command pool
    mCommandPool = nil;

    // Destroy device
    mHandle = nil;

    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

void MetalDevice::Configure(const RendererConfig& config)
{
#ifdef PLATFORM_MACOS
    mSwapchain.displaySyncEnabled = config.vsync ? YES : NO;
#endif
    
    auto oldFramesInFlight = mMaxFramesInFlight;
    if (mSwapchain.maximumDrawableCount >= 3 && config.tripleBufferingEnabled)
    {
        mMaxFramesInFlight = 3;
        GLEAM_CORE_TRACE("Metal: Triple buffering enabled.");
    }
    else if (mSwapchain.maximumDrawableCount >= 2)
    {
        mMaxFramesInFlight = 2;
        GLEAM_CORE_TRACE("Metal: Double buffering enabled.");
    }
    else
    {
        mMaxFramesInFlight = 1;
        GLEAM_ASSERT(false, "Metal: Neither triple nor double buffering is available!");
    }
    
    if (oldFramesInFlight != mMaxFramesInFlight)
    {
        DestroyPooledObjects();
    }
    mPooledObjects.resize(mMaxFramesInFlight);
}

id<CAMetalDrawable> MetalDevice::AcquireNextDrawable()
{
    if (mDrawable == nil)
    {
        dispatch_semaphore_wait(mImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
        mDrawable = [mSwapchain nextDrawable];
    }
    return mDrawable;
}

void MetalDevice::Present(const CommandBuffer* cmd)
{
    id<MTLCommandBuffer> commandBuffer = cmd->GetHandle();
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(mImageAcquireSemaphore);
    }];
    
    [commandBuffer presentDrawable:mDrawable];
    cmd->Commit();
    
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
    
    mDrawable = nil;
}

id<MTLCommandQueue> MetalDevice::GetCommandPool() const
{
    return mCommandPool;
}

#endif
