#pragma once
#include "Event.h"
#include "Math/Size.h"

namespace Gleam {

class RendererDeviceLostEvent : public Event
{
public:

	RendererDeviceLostEvent() = default;

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "RendererDeviceLostEvent";
		return ss.str();
	}

};

class RendererResizeEvent : public Event
{
public:

	RendererResizeEvent(const Size& size)
		: mSize(size) {}

    const Size& GetSize() const { return mSize; }

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "RendererResizeEvent: " << mSize.width << ", " << mSize.height;
		return ss.str();
	}

private:

    Size mSize;

};

} // namespace Gleam
