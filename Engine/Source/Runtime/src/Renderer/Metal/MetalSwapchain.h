#pragma once
#ifdef USE_METAL_RENDERER
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

namespace Gleam {

struct RendererConfig;
enum class TextureFormat;

class MetalSwapchain final
{
public:

	void Initialize(const RendererConfig& config);

	void Destroy();
    
    void Configure(const RendererConfig& config);

	id<CAMetalDrawable> AcquireNextDrawable();
	void Present(id<MTLCommandBuffer> commandBuffer);

    TextureFormat GetFormat() const;

	CAMetalLayer* GetHandle() const;
    
    const Size& GetSize() const;

private:

	uint32_t mMaxFramesInFlight = 3;

	uint32_t mCurrentFrameIndex = 0;

    Size mSize = Size::zero;
    
    void* mSurface = nullptr;
	
    dispatch_semaphore_t mImageAcquireSemaphore;
    
    id<CAMetalDrawable> mDrawable{ nil };

	CAMetalLayer* mHandle = nullptr;

};

} // namespace Gleam
#endif
