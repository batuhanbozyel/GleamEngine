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

	void Initialize();

	void Destroy();
    
    void Configure(const RendererConfig& config);

	id<CAMetalDrawable> AcquireNextDrawable();
	void Present(id<MTLCommandBuffer> commandBuffer);
    
    dispatch_semaphore_t GetSemaphore() const;

    TextureFormat GetFormat() const;

	CAMetalLayer* GetHandle() const;
    
    const Size& GetSize() const;
    
    uint32_t GetFrameIndex() const;
    
    uint32_t GetFramesInFlight() const;

private:

	uint32_t mMaxFramesInFlight = 3;

	uint32_t mCurrentFrameIndex = 0;

    Size mSize = Size::zero;
    
    MTLPixelFormat mImageFormat = MTLPixelFormatInvalid;
    
    void* mSurface = nullptr;
	
    dispatch_semaphore_t mImageAcquireSemaphore;
    
    id<CAMetalDrawable> mDrawable{ nil };

	CAMetalLayer* mHandle = nullptr;

};

} // namespace Gleam
#endif
