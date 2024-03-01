#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

struct Version;
struct RendererConfig;

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice();
    
    ~MetalDevice();
    
    id<CAMetalDrawable> AcquireNextDrawable();
    
    id<MTLCommandQueue> GetCommandPool() const;
    
private:

	virtual void Present(const CommandBuffer* cmd) override;

	virtual void Configure(const RendererConfig& config) override;
    
	void* mSurface = nullptr;

    dispatch_semaphore_t mImageAcquireSemaphore;

    id<CAMetalDrawable> mDrawable{ nil };

    CAMetalLayer* mSwapchain = nullptr;

    id<MTLCommandQueue> mCommandPool{ nil };

};

} // namespace Gleam
#endif
