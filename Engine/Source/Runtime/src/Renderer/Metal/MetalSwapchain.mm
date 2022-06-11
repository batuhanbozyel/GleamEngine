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

Swapchain::Swapchain(const TString& appName, const Version& appVersion, const RendererProperties& props)
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
    swapchain.name = [NSString stringWithCString:appName.c_str() encoding:NSASCIIStringEncoding];
    mHandle = swapchain;
    
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
    
    mContext.commandPool = [MetalDevice newCommandQueue];
    mContext.imageAcquireSemaphore = dispatch_semaphore_create(mProperties.maxFramesInFlight);
    
    mCommandBuffer = CreateScope<CommandBuffer>();
}

Swapchain::~Swapchain()
{
    mContext.drawable = nil;
    mContext.commandPool = nil;
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

NativeGraphicsHandle Swapchain::GetLayer() const
{
    return mHandle;
}

TextureFormat Swapchain::GetFormat() const
{
    return MTLPixelFormatToTextureFormat(((CAMetalLayer*)mHandle).pixelFormat);
}

const FrameObject& Swapchain::GetCurrentFrame() const
{
    return mContext.currentFrame;
}

void Swapchain::InvalidateAndCreate()
{
    CAMetalLayer* swapchain = (CAMetalLayer*)mHandle;
    
    int width, height;
    SDL_Metal_GetDrawableSize(Application::GetInstance().GetActiveWindow().GetSDLWindow(), &width, &height);
    mProperties.width = width;
    mProperties.height = height;
}

#endif
