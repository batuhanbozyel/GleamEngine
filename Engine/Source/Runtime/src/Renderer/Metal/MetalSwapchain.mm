#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalSwapchain.h"
#include "MetalUtils.h"

#include "Core/Window.h"
#include "Core/Game.h"
#include "Core/Events/RendererEvent.h"
#include "Renderer/RendererConfig.h"

#import <SDL_metal.h>
#import <QuartzCore/CAMetalLayer.h>

using namespace Gleam;

void MetalSwapchain::Initialize(const RendererConfig& config)
{
    // Create surface
    mSurface = SDL_Metal_CreateView(GameInstance->GetWindow()->GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    mHandle = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    mHandle.name = [NSString stringWithCString:GameInstance->GetWindow()->GetProperties().title.c_str() encoding:NSASCIIStringEncoding];
    mHandle.device = MetalDevice::GetHandle();
    mHandle.framebufferOnly = NO;
    mHandle.opaque = YES;
    Configure(config);
    
    mSize = GameInstance->GetWindow()->GetResolution();
    mHandle.frame.size = CGSizeMake(mSize.width, mSize.height);
    mHandle.drawableSize = CGSizeMake(mSize.width, mSize.height);
    
    EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
    {
        mSize.width = e.GetWidth();
        mSize.height = e.GetHeight();
        
        mHandle.frame.size = CGSizeMake(mSize.width, mSize.height);
        mHandle.drawableSize = CGSizeMake(mSize.width, mSize.height);
    });
    
    mImageAcquireSemaphore = dispatch_semaphore_create(mMaxFramesInFlight);
}

void MetalSwapchain::Destroy()
{
    mDrawable = nil;
    mHandle = nil;
}

void MetalSwapchain::Configure(const RendererConfig& config)
{
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
        GLEAM_ASSERT(false, "Metal: Neither triple nor double buffering is available!");
    }
}

id<CAMetalDrawable> MetalSwapchain::AcquireNextDrawable()
{
    if (mDrawable == nil)
    {
        dispatch_semaphore_wait(mImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
        mDrawable = [mHandle nextDrawable];
    }
    return mDrawable;
}

void MetalSwapchain::Present(id<MTLCommandBuffer> commandBuffer)
{
    [commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(mImageAcquireSemaphore);
    }];
    
    [commandBuffer presentDrawable:mDrawable];

    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mMaxFramesInFlight;
    
    mDrawable = nil;
}

TextureFormat MetalSwapchain::GetFormat() const
{
    return MTLPixelFormatToTextureFormat(mHandle.pixelFormat);
}

CAMetalLayer* MetalSwapchain::GetHandle() const
{
    return mHandle;
}

const Gleam::Size& MetalSwapchain::GetSize() const
{
    return mSize;
}

#endif
