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

	RendererResizeEvent(const Size& size)
		: mSize(size) {}

    const Size& GetSize() const { return mSize; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "RendererResizeEvent: " << mSize.width << ", " << mSize.height;
		return ss.str();
	}

private:

    Size mSize;

};

} // namespace Gleam
