#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/Swapchain.h"
#include "Renderer/RendererConfig.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

class MetalDevice;

class MetalSwapchain final : public Swapchain
{
	friend class MetalDevice;

public:

	MetalSwapchain();

	~MetalSwapchain();

	void Configure(MetalDevice* device, const RendererConfig& config);

	virtual const Texture& AcquireNextDrawable() override;

	virtual void Resize(GraphicsDevice* device, const Size& size) override;

	virtual void Present(const CommandBuffer* cmd) override;

private:
	
	Texture CreateSwapchainBuffer(uint32_t buffer);
	
	void* mSurface = nullptr;
	MetalDevice* mDevice = nullptr;
	CAMetalLayer* mHandle = nullptr;
	
	struct FrameContext
	{
		id<CAMetalDrawable> drawable = nil;
		id<MTLSharedEvent> event = nil;
		uint64_t eventValue = 0;
	};
	TArray<FrameContext> mContext;

};

} // Gleam
#endif
