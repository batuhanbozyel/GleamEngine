#pragma once
#include "Event.h"

namespace Gleam {

class RendererDeviceLostEvent : public Event
{
public:

	RendererDeviceLostEvent() = default;

};

class RendererResizeEvent : public Event
{
public:

	RendererResizeEvent(uint32_t width, uint32_t height)
		: mWidth(width), mHeight(height) {}

	uint32_t GetWidth() const { return mWidth; }
	uint32_t GetHeight() const { return mHeight; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "RendererDrawableResizeEvent: " << mWidth << ", " << mHeight;
		return ss.str();
	}

private:

	uint32_t mWidth, mHeight;

};

} // namespace Gleam