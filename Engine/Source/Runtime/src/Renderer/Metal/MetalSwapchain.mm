#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalSwapchain.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Core/Events/RendererEvent.h"

using namespace Gleam;

MetalSwapchain::MetalSwapchain()
{
    // init CAMetalLayer
    auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    
    // Create surface
    mSurface = SDL_Metal_CreateView(windowSystem->GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    mHandle = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    mHandle.name = [NSString stringWithCString:Globals::ProjectName.c_str() encoding:NSASCIIStringEncoding];
    mHandle.framebufferOnly = NO;
    mHandle.opaque = YES;
}

MetalSwapchain::~MetalSwapchain()
{
    for (uint32_t i = 0; i < mMaxFramesInFlight; ++i)
    {
        dispatch_semaphore_wait(mImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
    }
    
    // we need to revert back to its initial value for some reason????
    for (uint32_t i = 0; i < mMaxFramesInFlight; ++i)
    {
        dispatch_semaphore_signal(mImageAcquireSemaphore);
    }
    
    mImageAcquireSemaphore = nil;
    mHandle = nil;
    mSurface = nil;
    mTextures.clear();
    
    SDL_Metal_DestroyView(mSurface);
    mSurface = nil;
}

void MetalSwapchain::Configure(MetalDevice* device, const RendererConfig& config)
{
    mHandle.device = device->GetHandle();
	mCurrentFrameIndex = 0;
    mDevice = device;
    
#ifdef PLATFORM_MACOS
    mHandle.displaySyncEnabled = config.vsync ? YES : NO;
#endif
    
    if (mHandle.maximumDrawableCount >= 3 && config.tripleBufferingEnabled)
    {
        mMaxFramesInFlight = 3;
        GLEAM_CORE_TRACE("Metal: Triple buffering enabled.");
    }
    else if (mHandle.maximumDrawableCount >= 2)
    {
        mMaxFramesInFlight = 2;
        GLEAM_CORE_TRACE("Metal: Double buffering enabled.");
    }
    else
    {
        mMaxFramesInFlight = 1;
        GLEAM_ASSERT(false, "Metal: Neither triple nor double buffering is available.");
    }
    mTextures.resize(mMaxFramesInFlight);
    mImageAcquireSemaphore = dispatch_semaphore_create(mMaxFramesInFlight);
    
    int width, height;
    auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    SDL_GetWindowSizeInPixels(windowSystem->GetSDLWindow(), &width, &height);
    Resize(device, Size((float)width, (float)height));
}

void MetalSwapchain::Resize(GraphicsDevice* device, const Size& size)
{
    auto physicalSize = size * mHandle.contentsScale;
    mHandle.drawableSize = CGSizeMake(physicalSize.width, physicalSize.height);
    
    for (uint32_t i = 0; i < mMaxFramesInFlight; ++i)
    {
        mTextures[i] = CreateSwapchainBuffer(i);
    }
}

const Texture& MetalSwapchain::AcquireNextDrawable()
{
    auto& texture = mTextures[mCurrentFrameIndex];
    dispatch_semaphore_wait(mImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
    id<CAMetalDrawable> drawable = [mHandle nextDrawable];
    texture = Texture(texture.GetDescriptor(), drawable, drawable.texture);
    return texture;
}

void MetalSwapchain::Present(const CommandBuffer* cmd)
{
    auto& texture = mTextures[mCurrentFrameIndex];
    id<CAMetalDrawable> drawable = texture.GetHandle();
    id<MTL4CommandQueue> commandQueue = mDevice->GetCommandQueue();
    
    cmd->End();
    [commandQueue waitForDrawable:drawable];
    cmd->Commit();
    [commandQueue signalDrawable:drawable];
    
    [drawable addPresentedHandler:^(id<MTLDrawable> drawable)
    {
        dispatch_semaphore_signal(mImageAcquireSemaphore);
    }];
    [drawable present];
    
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
}

Texture MetalSwapchain::CreateSwapchainBuffer(uint32_t buffer)
{
    TStringStream resourceName;
    resourceName << "Swapchain::Drawable_" << buffer;
    
    TextureDescriptor swapchainDesc;
    swapchainDesc.name = resourceName.str();
    swapchainDesc.dimension = TextureDimension::Texture2D;
    swapchainDesc.size = Size(mHandle.drawableSize.width, mHandle.drawableSize.height);
    swapchainDesc.usage = TextureUsage_Attachment;
    swapchainDesc.format = MTLPixelFormatToTextureFormat(mHandle.pixelFormat);
    return Texture(swapchainDesc);
}

#endif
