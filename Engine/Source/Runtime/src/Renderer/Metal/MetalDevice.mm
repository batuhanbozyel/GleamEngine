#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "MetalPipelineStateManager.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Core/Events/RendererEvent.h"

#define IR_PRIVATE_IMPLEMENTATION
#include <metal_irconverter_runtime/metal_irconverter_runtime.h>

using namespace Gleam;

void RenderSystem::InitializeBackend()
{
	mSwapchain = CreateScope<DirectXSwapchain>();
	mDevice = CreateScope<DirectXDevice>(mSwapchain.get());
	mUploadManager = CreateScope<UploadManager>(mDevice.get());
	mReleaseQueue = CreateScope<ResourceReleaseQueue>(mSwapchain->GetFramesInFlight());
	mResourcePool = CreateScope<RenderResourcePool>(mDevice.get(), mSwapchain.get(), mReleaseQueue.get());
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
    IRCompilerSetGlobalRootSignature(compiler, MetalPipelineStateManager::GetGlobalRootSignature());
    
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
    
    if (texture.GetDescriptor().sampleCount > 1)
    {
        texture.mMultisampleHandle = nil;
        texture.mMultisampleView = nil;
    }
}

void GraphicsDevice::Dispose(Shader& shader)
{
	shader.mHandle = nil;
}

MetalDevice::MetalDevice()
{
    // init MTLDevice
    mHandle = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mHandle);

    // init CAMetalLayer
    auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    
    // Create surface
    mSurface = SDL_Metal_CreateView(windowSystem->GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    mSwapchain = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    mSwapchain.name = [NSString stringWithCString:Globals::ProjectName.c_str() encoding:NSASCIIStringEncoding];
    mSwapchain.device = mHandle;
    mSwapchain.framebufferOnly = NO;
    mSwapchain.opaque = YES;
    
    const auto& resolution = Globals::Engine->GetResolution();
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
    cmd->End();
	cmd->Commit();
    
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
    
    mDrawable = nil;
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
    IRDescriptorTableSetTexture(descriptorTable + index.data, texture.GetView(), 0.0f, 0);
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

#endif
