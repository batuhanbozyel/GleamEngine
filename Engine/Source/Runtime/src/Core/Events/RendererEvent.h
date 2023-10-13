#pragma once
#include "Event.h"

namespace Gleam {

class RendererDeviceLostEvent : public Event
{
public:

	RendererDeviceLostEvent() = default;

};

class RendererPresentEvent : public Event
{
public:
    
    RendererPresentEvent(uint32_t frameIdx)
        : mFrameIndex(frameIdx)
    {
        
    }
    
    uint32_t GetFrameIndex() const
    {
        return mFrameIndex;
    }
    
private:
    
    uint32_t mFrameIndex;
    
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
