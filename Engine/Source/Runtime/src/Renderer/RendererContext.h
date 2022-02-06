#pragma once
#include "Swapchain.h"

namespace Gleam {

struct Version;

class RendererContext
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static NativeGraphicsHandle GetEntryPoint();

	static NativeGraphicsHandle GetDevice();

	static NativeGraphicsHandle GetPhysicalDevice();

	static NativeGraphicsHandle GetGraphicsQueue();

	static NativeGraphicsHandle GetComputeQueue();

	static NativeGraphicsHandle GetTransferQueue();

	static uint32_t GetGraphicsQueueIndex();

	static uint32_t GetComputeQueueIndex();

	static uint32_t GetTransferQueueIndex();

	static const Scope<Swapchain>& GetSwapchain()
	{
		return mSwapchain;
	}

	static const RendererProperties& GetProperties()
	{
		return mSwapchain->GetProperties();
	}

	static bool IsMultisampleEnabled()
	{
		return mSwapchain->GetProperties().msaa > 1;
	}

private:
    
	static inline Scope<Swapchain> mSwapchain = nullptr;

};

} // namespace Gleam
