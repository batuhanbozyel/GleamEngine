#pragma once
#include "Event.h"
#include "Input/MouseButton.h"

namespace Gleam {

class MouseButtonEvent : public Event
{
public:

    MouseButton GetMouseButton() const { return mButton; }

protected:

	MouseButtonEvent(MouseButton button)
		: mButton(button) {}

    MouseButton mButton;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:

	MouseButtonPressedEvent(MouseButton button)
		: MouseButtonEvent(button) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseButtonPressedEvent: " << Gleam::ToString(mButton);
		return ss.str();
	}
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:

	MouseButtonReleasedEvent(MouseButton button)
		: MouseButtonEvent(button) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseButtonReleasedEvent: " << Gleam::ToString(mButton);
		return ss.str();
	}
};

class MouseMovedEvent : public Event
{
public:

	MouseMovedEvent(float xpos, float ypos)
		: mPosition(xpos, ypos) {}


	const Float2& GetPosition() const { return mPosition; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseMovedEvent: " << mPosition.x << ", " << mPosition.y;
		return ss.str();
	}

private:

	Float2 mPosition;
};

class MouseScrolledEvent : public Event
{
public:

	MouseScrolledEvent(float xoffset, float yoffset)
		: mScrollOffset(xoffset, yoffset) {}

	const Float2& GetOffset() const { return mScrollOffset; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseScrolledEvent: " << mScrollOffset.x << ", " << mScrollOffset.y;
		return ss.str();
	}

private:

	Float2 mScrollOffset;
};

} // namespace Gleam
