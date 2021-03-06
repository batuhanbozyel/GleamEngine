#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "Renderer/RendererContext.h"
#include "MetalPipelineStateManager.h"

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
    
    mSwapchain.reset(new Swapchain(props, nil));
    CAMetalLayer* swapchain = mSwapchain->GetHandle();
    swapchain.name = [NSString stringWithCString:appName.c_str() encoding:NSASCIIStringEncoding];
    mSwapchain->InvalidateAndCreate();
    
    mContext.commandPool = [MetalDevice newCommandQueue];
    
    GLEAM_CORE_INFO("Metal: Graphics context created.");
}
/************************************************************************/
/*	Destroy                                                             */
/************************************************************************/
void RendererContext::Destroy()
{
    MetalPipelineStateManager::Clear();
    
    // Destroy swapchain
    mSwapchain.reset();
    
    // Destroy device
    mDevice = nil;
    
    GLEAM_CORE_INFO("Metal: Graphics context destroyed.");
}
/************************************************************************/
/*    GetPhysicalDevice                                                 */
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
    return mContext.commandPool;
}
/************************************************************************/
/*    GetComputeQueue                                                     */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetComputeQueue()
{
    return mContext.commandPool;
}
/************************************************************************/
/*    GetTransferQueue                                                    */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetTransferQueue()
{
    return mContext.commandPool;
}
/************************************************************************/
/*	GetGraphicsCommandPool                                              */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetGraphicsCommandPool(uint32_t index)
{
	return mContext.commandPool;
}
/************************************************************************/
/*    GetTransferCommandPool                                              */
/************************************************************************/
NativeGraphicsHandle RendererContext::GetTransferCommandPool(uint32_t index)
{
    return mContext.commandPool;
}
#endif
