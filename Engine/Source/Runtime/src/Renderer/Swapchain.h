#pragma once
#include "RenderSurface.h"

namespace Gleam {

class Swapchain : public RenderSurface
{
public:

	virtual ~Swapchain() = default;

	virtual const Texture& AcquireNextDrawable() = 0;

	const Texture& GetCurrentDrawable() const
	{
		GLEAM_ASSERT(mTextures.size() == mMaxFramesInFlight, "Swapchain is not configured properly.");
		return mTextures[mCurrentFrameIndex];
	}

	uint32_t GetFrameIndex() const
	{
		return mCurrentFrameIndex;
	}

	uint32_t GetFramesInFlight() const
	{
		return mMaxFramesInFlight;
	}

protected:

	uint32_t mMaxFramesInFlight = 3;

	uint32_t mCurrentFrameIndex = 0;

	TArray<Texture> mTextures;

};

} // namespace Gleam
