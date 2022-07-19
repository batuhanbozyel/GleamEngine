#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/Swapchain.h"
#include "MetalUtils.h"

#include "Core/Window.h"
#include "Core/Application.h"

#import <SDL_metal.h>
#import <QuartzCore/CAMetalLayer.h>

using namespace Gleam;

struct
{
    // Synchronization
    dispatch_semaphore_t imageAcquireSemaphore;
    
    // Frame
    id<CAMetalDrawable> drawable{ nil };
} mContext;

Swapchain::Swapchain(const RendererProperties& props, NativeGraphicsHandle instance)
    : mProperties(props)
{
    // Create surface
    mSurface = SDL_Metal_CreateView(ApplicationInstance.GetActiveWindow().GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    CAMetalLayer* swapchain = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    swapchain.device = MetalDevice;
    swapchain.framebufferOnly = YES;
    swapchain.opaque = YES;
    swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
    swapchain.drawableSize = swapchain.frame.size;
    mHandle = swapchain;
    
    mProperties.width = swapchain.frame.size.width;
    mProperties.height = swapchain.frame.size.height;
    
    if (swapchain.maximumDrawableCount >= 3 && mProperties.tripleBufferingEnabled)
    {
        mProperties.maxFramesInFlight = 3;
        GLEAM_CORE_TRACE("Metal: Triple buffering enabled.");
    }
    else if (swapchain.maximumDrawableCount >= 2)
    {
        mProperties.maxFramesInFlight = 2;
        if (mProperties.tripleBufferingEnabled)
        {
            mProperties.tripleBufferingEnabled = false;
        }
        GLEAM_CORE_TRACE("Metal: Double buffering enabled.");
    }
    else
    {
        mProperties.maxFramesInFlight = 1;
        GLEAM_ASSERT(false, "Metal: Neither triple nor double buffering is available!");
    }
    
    mContext.imageAcquireSemaphore = dispatch_semaphore_create(mProperties.maxFramesInFlight);
}

Swapchain::~Swapchain()
{
    mContext.drawable = nil;
    mHandle = nil;
}

NativeGraphicsHandle Swapchain::AcquireNextDrawable()
{
    dispatch_semaphore_wait(mContext.imageAcquireSemaphore, DISPATCH_TIME_FOREVER);
    mContext.drawable = [(CAMetalLayer*)mHandle nextDrawable];
    return mContext.drawable;
}

void Swapchain::Present(NativeGraphicsHandle commandBuffer)
{
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(mContext.imageAcquireSemaphore);
    }];
    
    [commandBuffer presentDrawable:mContext.drawable];
    
    [commandBuffer commit];

    InvalidateAndCreate();

    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mProperties.maxFramesInFlight;
}

TextureFormat Swapchain::GetFormat() const
{
    return MTLPixelFormatToTextureFormat(((CAMetalLayer*)mHandle).pixelFormat);
}

NativeGraphicsHandle Swapchain::GetDrawable() const
{
    return mContext.drawable;
}

DispatchSemaphore Swapchain::GetImageAcquireSemaphore() const
{
    return mContext.imageAcquireSemaphore;
}

DispatchSemaphore Swapchain::GetImageReleaseSemaphore() const
{
    return mContext.imageAcquireSemaphore;
}

void Swapchain::InvalidateAndCreate()
{
    CAMetalLayer* swapchain = (CAMetalLayer*)mHandle;
    if (mProperties.width != swapchain.frame.size.width || mProperties.height != swapchain.frame.size.height)
    {
        mProperties.width = swapchain.frame.size.width;
        mProperties.height = swapchain.frame.size.height;
        swapchain.drawableSize = swapchain.frame.size;
    }
}

#endif
