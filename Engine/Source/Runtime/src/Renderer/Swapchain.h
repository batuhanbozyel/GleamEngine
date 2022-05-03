#pragma once
#include "RendererConfig.h"
#include "TextureFormat.h"
#include "CommandBuffer.h"

namespace Gleam {

struct Version;

#ifdef USE_METAL_RENDERER
struct MetalFrameObject;
using FrameObject = MetalFrameObject;
#else
struct VulkanFrameObject;
using FrameObject = VulkanFrameObject;
#endif

class Swapchain : public GraphicsObject
{
public:

	Swapchain(const TString& appName, const Version& appVersion, const RendererProperties& props);
	~Swapchain();

	void InvalidateAndCreate();

	const FrameObject& AcquireNextFrame();
	void Present();

	NativeGraphicsHandle GetGraphicsCommandPool(uint32_t index) const;
    NativeGraphicsHandle GetLayer() const;
    TextureFormat GetFormat() const;
    const FrameObject& GetCurrentFrame() const;
    
	const CommandBuffer& GetCommandBuffer() const
    {
        return *mCommandBuffer;
    }

	uint32_t GetCurrentFrameIndex() const
	{
		return mCurrentFrameIndex;
	}

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

private:

	uint32_t mCurrentFrameIndex = 0;

	RendererProperties mProperties;
    
    void* mSurface = nullptr;
    Scope<CommandBuffer> mCommandBuffer = nullptr;

};

} // namespace Gleam
