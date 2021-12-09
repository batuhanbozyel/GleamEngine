#pragma once
#include "Event.h"

namespace Gleam {

class MouseButtonEvent : public Event
{
public:

	int GetMouseButton() const { return mMouseCode; }

protected:

	MouseButtonEvent(int mouseCode)
		: mMouseCode(mouseCode) {}

	int mMouseCode;
};

class MouseButtonPressedEvent : public MouseButtonEvent
{
public:

	MouseButtonPressedEvent(int mouseCode)
		: MouseButtonEvent(mouseCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseButtonPressedEvent: " << mMouseCode;
		return ss.str();
	}
};

class MouseButtonReleasedEvent : public MouseButtonEvent
{
public:

	MouseButtonReleasedEvent(int mouseCode)
		: MouseButtonEvent(mouseCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseButtonReleasedEvent: " << mMouseCode;
		return ss.str();
	}
};

class MouseMovedEvent : public Event
{
public:

	MouseMovedEvent(float xpos, float ypos)
		: mMousePos(xpos, ypos) {}


	const Vector2& GetPos() const { return mMousePos; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseMovedEvent: " << mMousePos.x << ", " << mMousePos.y;
		return ss.str();
	}

private:

	Vector2 mMousePos;
};

class MouseScrolledEvent : public Event
{
public:

	MouseScrolledEvent(float xoffset, float yoffset)
		: mMouseScrollOffset(xoffset, yoffset) {}

	const Vector2& GetOffset() const { return mMouseScrollOffset; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "MouseScrolledEvent: " << mMouseScrollOffset.x << ", " << mMouseScrollOffset.y;
		return ss.str();
	}

private:

	Vector2 mMouseScrollOffset;
};

} // namespace Gleam