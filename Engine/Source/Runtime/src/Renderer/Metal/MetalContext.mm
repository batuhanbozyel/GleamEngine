#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "MetalUtils.h"

using namespace Gleam;

struct
{
    // Command Pool
    id<MTLCommandQueue> commandPool;
} mContext;

/************************************************************************/
/*	Init                                      */
/************************************************************************/
void RendererContext::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
    mDevice = MTLCreateSystemDefaultDevice();
    GLEAM_ASSERT(mDevice);
    
    mSwapchain.reset(new Swapchain(appName, appVersion, props));
    mSwapchain->InvalidateAndCreate();
    
    GLEAM_CORE_INFO("Metal: Graphics context created.");
}
/************************************************************************/
/*	Destroy                                   */
/************************************************************************/
void RendererContext::Destroy()
{
    // Destroy swapchain
    mSwapchain.reset();
    
    // Destroy device
    mDevice = nil;
    
    GLEAM_CORE_INFO("Metal: Graphics context destroyed.");
}
/************************************************************************/
/*    GetPhysicalDevice                                                   */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetPhysicalDevice()
{
    return mDevice;
}
/************************************************************************/
/*	GetGraphicsCommandPool                                              */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetGraphicsCommandPool(uint32_t index)
{
	return mContext.commandPool;
}
#endif
