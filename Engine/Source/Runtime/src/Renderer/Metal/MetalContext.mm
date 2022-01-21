#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "MetalUtils.h"

#include <SDL_metal.h>
#import <AppKit/NSView.h>

#include "Core/Window.h"
#include "Core/Application.h"
#include "Core/Events/WindowEvent.h"

using namespace Gleam;

struct
{
    // Surface
    SDL_MetalView Surface;
    
    // Device
    id<MTLDevice> Device;
    
    // Swapchain
    CAMetalLayer* Swapchain;
    
    // Synchronization
    dispatch_semaphore_t ImageAcquireSemaphore;
    
    // Frame
    id<MTLCommandQueue> CommandPool;
    MetalFrameObject CurrentFrame;
} mContext;

/************************************************************************/
/*	RendererContext                           */
/************************************************************************/
RendererContext::RendererContext(const TString& appName, const Version& appVersion, const RendererProperties& props)
    : mProperties(props)
{
    mContext.Surface = SDL_Metal_CreateView(Application::GetActiveWindow().GetSDLWindow());
    mContext.Device = MTLCreateSystemDefaultDevice();
    
    mContext.Swapchain = (__bridge CAMetalLayer*)(SDL_Metal_GetLayer(mContext.Surface));
    mContext.Swapchain.device = mContext.Device;
    mContext.Swapchain.pixelFormat = MTLPixelFormatBGRA8Unorm;
    mContext.Swapchain.framebufferOnly = YES;
    mContext.Swapchain.opaque = YES;
    
    int width, height;
    SDL_Metal_GetDrawableSize(Application::GetActiveWindow().GetSDLWindow(), &width, &height);
    mProperties.width = width / mContext.Swapchain.contentsScale;
    mProperties.height = height / mContext.Swapchain.contentsScale;
    
    CGSize size;
    size.width = mProperties.width;
    size.height = mProperties.height;
    mContext.Swapchain.drawableSize = size;
    
    if (mContext.Swapchain.maximumDrawableCount >= 3 && mProperties.tripleBufferingEnabled)
    {
        mProperties.maxFramesInFlight = 3;
        GLEAM_CORE_TRACE("Metal: Triple buffering enabled.");
    }
    else if (mContext.Swapchain.maximumDrawableCount >= 2)
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
    
    mContext.CommandPool = [mContext.Device newCommandQueue];
    mContext.ImageAcquireSemaphore = dispatch_semaphore_create(mProperties.maxFramesInFlight);
}
/************************************************************************/
/*	~RendererContext                          */
/************************************************************************/
RendererContext::~RendererContext()
{
    mContext.CommandPool = nil;
    mContext.Swapchain = nil;
    mContext.Device = nil;
    SDL_Metal_DestroyView(mContext.Surface);
}
/************************************************************************/
/*    GetDevice                               */
/************************************************************************/
handle_t RendererContext::GetDevice() const
{
    return (__bridge handle_t)mContext.Device;
}
/************************************************************************/
/*    InvalidateSwapchain                     */
/************************************************************************/
void RendererContext::InvalidateSwapchain()
{
    int width, height;
    SDL_Metal_GetDrawableSize(Application::GetActiveWindow().GetSDLWindow(), &width, &height);
    mProperties.width = width / mContext.Swapchain.contentsScale;
    mProperties.height = height / mContext.Swapchain.contentsScale;
}
/************************************************************************/
/*    AcquireNextFrame                        */
/************************************************************************/
const MetalFrameObject& RendererContext::AcquireNextFrame()
{
    dispatch_semaphore_wait(mContext.ImageAcquireSemaphore, DISPATCH_TIME_FOREVER);
    
    mContext.CurrentFrame.drawable = [mContext.Swapchain nextDrawable];
    mContext.CurrentFrame.commandBuffer = [mContext.CommandPool commandBuffer];
    
    return mContext.CurrentFrame;
}
/************************************************************************/
/*    Present                                 */
/************************************************************************/
void RendererContext::Present()
{
    [mContext.CurrentFrame.commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> commandBuffer)
    {
        dispatch_semaphore_signal(mContext.ImageAcquireSemaphore);
    }];
    
    [mContext.CurrentFrame.commandBuffer presentDrawable:mContext.CurrentFrame.drawable];
    [mContext.CurrentFrame.commandBuffer commit];
    
    InvalidateSwapchain();
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mProperties.maxFramesInFlight;
}
/************************************************************************/
/*    GetCurrentFrame                         */
/************************************************************************/
const MetalFrameObject& RendererContext::GetCurrentFrame() const
{
    return mContext.CurrentFrame;
}
#endif
