#pragma once
#ifdef USE_METAL_RENDERER
#include "Renderer/Swapchain.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

class MetalDevice;
struct RendererConfig;
enum class TextureFormat;

class MetalSwapchain final : public Swapchain
{
public:
    
    void Initialize(MetalDevice* device);

    void Destroy();

    void Configure(const RendererConfig& config);;

    id<CAMetalDrawable> AcquireNextDrawable();
    void Present(id<MTLCommandBuffer> commandBuffer);

    dispatch_semaphore_t GetSemaphore() const;

    CAMetalLayer* GetHandle() const;

private:
    
    void* mSurface = nullptr;

    dispatch_semaphore_t mImageAcquireSemaphore;

    id<CAMetalDrawable> mDrawable{ nil };

    CAMetalLayer* mHandle = nullptr;

};

} // namespace Gleam
#endif
