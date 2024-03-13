#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/GraphicsDevice.h"

#include <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

struct Version;
struct RendererConfig;

struct MetalDescriptorHeap
{
    ResourceDescriptorHeap heap;
    id<MTLBuffer> handle;
};

class MetalDevice final : public GraphicsDevice
{
public:
    
    MetalDevice();
    
    ~MetalDevice();
    
    id<MTLBuffer> GetCbvSrvUavHeap() const;
    
    id<CAMetalDrawable> AcquireNextDrawable();
    
    id<MTLCommandQueue> GetCommandPool() const;
    
    virtual ShaderResourceIndex CreateResourceView(const Buffer& buffer) override;
    
    virtual ShaderResourceIndex CreateResourceView(const Texture& texture) override;
    
    virtual void ReleaseResourceView(ShaderResourceIndex view) override;
    
private:

	virtual void Present(const CommandBuffer* cmd) override;

	virtual void Configure(const RendererConfig& config) override;
    
    MetalDescriptorHeap CreateDescriptorHeap(uint32_t capacity) const;
    
	void* mSurface = nullptr;

    dispatch_semaphore_t mImageAcquireSemaphore;

    id<CAMetalDrawable> mDrawable{ nil };

    CAMetalLayer* mSwapchain = nullptr;

    id<MTLCommandQueue> mCommandPool{ nil };
    
    MetalDescriptorHeap mCbvSrvUavHeap;

};

} // namespace Gleam
#endif
