#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalSwapchain.h"
#include "MetalDevice.h"
#include "MetalUtils.h"

#include "Core/WindowSystem.h"
#include "Core/Application.h"
#include "Core/Events/RendererEvent.h"
#include "Renderer/RendererConfig.h"

#import <SDL3/SDL_metal.h>
#import <QuartzCore/CAMetalLayer.h>

using namespace Gleam;

void MetalSwapchain::Initialize(MetalDevice* device)
{
    auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();
    
    // Create surface
    mSurface = SDL_Metal_CreateView(windowSystem->GetSDLWindow());
    GLEAM_ASSERT(mSurface, "Metal: Surface creation failed!");
    
    mHandle = (__bridge CAMetalLayer*)SDL_Metal_GetLayer(mSurface);
    mHandle.name = [NSString stringWithCString:windowSystem->GetProperties().title.c_str() encoding:NSASCIIStringEncoding];
    mHandle.device = device->GetHandle();
    mHandle.framebufferOnly = NO;
    mHandle.opaque = YES;
    
    const auto& resolution = windowSystem->GetResolution();
    mSize = resolution * mHandle.contentsScale;
    mHandle.frame.size = CGSizeMake(mSize.width, mSize.height);
    mHandle.drawableSize = CGSizeMake(resolution.width, resolution.height);
    mFormat = MTLPixelFormatToTextureFormat(mHandle.pixelFormat);
    
    EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
    {
        mSize.width = e.GetWidth() * mHandle.contentsScale;
        mSize.height = e.GetHeight() * mHandle.contentsScale;
        
        mHandle.frame.size = CGSizeMake(e.GetWidth(), e.GetHeight());
        mHandle.drawableSize = CGSizeMake(mSize.width, mSize.height);
    });
    
    mImageAcquireSemaphore = dispatch_semaphore_create(mMaxFramesInFlight);
}

void MetalSwapchain::Destroy()
{
    ClearAll();
    mImageAcquireSemaphore = nil;
    mDrawable = nil;
    mHandle = nil;
    mSurface = nil;
}

void MetalSwapchain::Configure(const RendererConfig& config)
{
    ClearAll();
    
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
    mPooledObjects.resize(mMaxFramesInFlight);
    EventDispatcher<RendererResizeEvent>::Publish(RendererResizeEvent(mSize));
}

id<CAMetalDrawable> MetalSwapchain::AcquireNextDrawable()
{
    if (mDrawable == nil)
    {
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
    
    dispatch_semaphore_wait(mImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
    
    Clear();
    
    mDrawable = nil;
}

dispatch_semaphore_t MetalSwapchain::GetSemaphore() const
{
    return mImageAcquireSemaphore;
}

CAMetalLayer* MetalSwapchain::GetHandle() const
{
    return mHandle;
}

#endif
