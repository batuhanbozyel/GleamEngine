#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "MetalUtils.h"

using namespace Gleam;

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
/*    GetGraphicsQueue                                                    */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetGraphicsQueue()
{
    return nullptr;
}
/************************************************************************/
/*    GetComputeQueue                                                     */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetComputeQueue()
{
    return nullptr;
}
/************************************************************************/
/*    GetTransferQueue                                                    */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetTransferQueue()
{
    return nullptr;
}
/************************************************************************/
/*    GetGraphicsQueueIndex                                               */
/************************************************************************/
uint32_t RendererContext::GetGraphicsQueueIndex()
{
    return 0;
}
/************************************************************************/
/*    GetComputeQueueIndex                                                */
/************************************************************************/
uint32_t RendererContext::GetComputeQueueIndex()
{
    return 0;
}
/************************************************************************/
/*    GetTransferQueueIndex                                               */
/************************************************************************/
uint32_t RendererContext::GetTransferQueueIndex()
{
    return 0;
}
/************************************************************************/
/*    GetMemoryTypeForProperties                                           */
/************************************************************************/
uint32_t RendererContext::GetMemoryTypeForProperties(uint32_t memoryTypeBits, uint32_t properties)
{
    return 0;
}
#endif
