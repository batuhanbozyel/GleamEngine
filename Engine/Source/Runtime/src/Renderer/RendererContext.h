#pragma once
#include "Swapchain.h"

namespace Gleam {

struct Version;

class RendererContext
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static void WaitIdle();

	static NativeGraphicsHandle GetPhysicalDevice();

	static NativeGraphicsHandle GetGraphicsQueue();

	static NativeGraphicsHandle GetComputeQueue();

	static NativeGraphicsHandle GetTransferQueue();

	static NativeGraphicsHandle GetGraphicsCommandPool(uint32_t index);

	static uint32_t GetMemoryTypeForProperties(uint32_t memoryTypeBits, uint32_t properties);

    static NativeGraphicsHandle GetDevice()
    {
        return mDevice;
    }
    
	static const Scope<Swapchain>& GetSwapchain()
	{
		return mSwapchain;
	}

	static const RendererProperties& GetProperties()
	{
		return mSwapchain->GetProperties();
	}

private:
    
    static inline NativeGraphicsHandle mDevice = nullptr;
    
	static inline Scope<Swapchain> mSwapchain = nullptr;

};

} // namespace Gleam
