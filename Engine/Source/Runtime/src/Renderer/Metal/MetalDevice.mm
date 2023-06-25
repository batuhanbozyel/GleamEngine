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

        // init MTLLibrary
        NSError* error;
        auto binaryData = IOUtils::ReadBinaryFile(GameInstance->GetDefaultAssetPath().append("PrecompiledShaders.metallib"));
        mShaderLibrary = [mHandle newLibraryWithData:dispatch_data_create(binaryData.data(), binaryData.size(), nil, DISPATCH_DATA_DESTRUCTOR_DEFAULT) error:&error];
        GLEAM_ASSERT(mShaderLibrary);

        GLEAM_CORE_INFO("Metal: Graphics device created.");
    }
}

void MetalDevice::Destroy()
{
    MetalPipelineStateManager::Clear();
    
    // Destroy command pool
    mCommandPool = nil;

    // Destroy library
    mShaderLibrary = nil;
    
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

id<MTLLibrary> MetalDevice::GetShaderLibrary()
{
    return mShaderLibrary;
}

id<MTLDevice> MetalDevice::GetHandle()
{
    return mHandle;
}

#endif