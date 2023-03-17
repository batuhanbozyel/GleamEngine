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

	void Initialize(const TString& appName, const RendererConfig& config);

	void Destroy();

	void UpdateSize();

	id<CAMetalDrawable> AcquireNextDrawable();
	void Present(id<MTLCommandBuffer> commandBuffer);

	id<CAMetalDrawable> GetDrawable() const;

    TextureFormat GetFormat() const;

	CAMetalLayer* GetHandle() const;

	dispatch_semaphore_t GetImageAcquireSemaphore() const;

	dispatch_semaphore_t GetImageReleaseSemaphore() const;
    
    const Size& GetSize() const;

	uint32_t GetFrameIndex() const;

	uint32_t GetMaxFramesInFlight() const;

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
