#include "gpch.h"

#ifdef USE_METAL_RENDERER
#include "MetalDevice.h"
#include "MetalPipelineStateManager.h"

#include "Core/Application.h"

using namespace Gleam;

void MetalDevice::Init()
{
    if (mHandle == nil)
    {
        // init MTLDevice
        mHandle = MTLCreateSystemDefaultDevice();
        GLEAM_ASSERT(mHandle);

        // init CAMetalLayer
        mSwapchain.Initialize();

        // init MTLCommandQueue
        mCommandPool = [mHandle newCommandQueue];
        
        MetalPipelineStateManager::Init();

        GLEAM_CORE_INFO("Metal: Graphics device created.");
    }
}

void MetalDevice::Destroy()
{
    MetalPipelineStateManager::Destroy();
    
    // Destroy command pool
    mCommandPool = nil;
    
    // Destroy swapchain
    mSwapchain.Destroy();

    // Destroy device
    mHandle = nil;
    
    GLEAM_CORE_INFO("Metal: Graphics device destroyed.");
}

MetalSwapchain& MetalDevice::GetSwapchain()
{
    return mSwapchain;
}

id<MTLCommandQueue> MetalDevice::GetCommandPool()
{
    return mCommandPool;
}

id<MTLDevice> MetalDevice::GetHandle()
{
    return mHandle;
}

#endif
