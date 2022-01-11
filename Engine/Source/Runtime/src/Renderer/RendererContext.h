#pragma once
#include "RendererConfig.h"

namespace Gleam {

struct Version;

class RendererContext
{
public:

	RendererContext(const TString& appName, const Version& appVersion, const RendererProperties& props);
	~RendererContext();

	void BeginFrame();
	void ClearScreen(const Color& color) const;
	void EndFrame();

    void* GetDevice() const;

	const RendererProperties& GetProperties() const
	{
		return mProperties;
	}

private:
    
    void InvalidateSwapchain();
    
	RendererProperties mProperties;
    uint32_t mCurrentFrameIndex = 0;
};

} // namespace Gleam
