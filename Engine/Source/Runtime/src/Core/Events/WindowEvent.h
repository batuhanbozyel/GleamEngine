#pragma once
#include "Event.h"

namespace Gleam {

class WindowEvent : public Event
{
public:

	WindowEvent(uint32_t windowID)
		: m_Window(windowID) {}

	uint32_t GetWindowID() const
	{
		return m_Window;
	}

private:

	uint32_t m_Window;
};

class WindowCloseEvent : public WindowEvent
{
public:
	WindowCloseEvent(uint32_t windowID)
		: WindowEvent(windowID) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowCloseEvent";
		return ss.str();
	}
};

class WindowResizeEvent : public WindowEvent
{
public:

	WindowResizeEvent(uint32_t windowID, uint32_t width, uint32_t height)
		: WindowEvent(windowID), m_Width(width), m_Height(height) {}

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
		return ss.str();
	}

private:

	uint32_t m_Width, m_Height;
};

class WindowMaximizeEvent : public WindowEvent
{
public:

	WindowMaximizeEvent(uint32_t windowID)
		: WindowEvent(windowID) {}

};

class WindowMinimizeEvent : public WindowEvent
{
public:

	WindowMinimizeEvent(uint32_t windowID)
		: WindowEvent(windowID) {}

};

class WindowRestoreEvent : public WindowEvent
{
public:

	WindowRestoreEvent(uint32_t windowID)
		: WindowEvent(windowID)
	{
	}

};

class WindowFocusEvent : public WindowEvent
{
public:

	WindowFocusEvent(uint32_t windowID)
		: WindowEvent(windowID) {}

};

class WindowLostFocusEvent : public WindowEvent
{
public:

	WindowLostFocusEvent(uint32_t windowID)
		: WindowEvent(windowID) {}

};

class WindowMovedEvent : public WindowEvent
{
public:

	WindowMovedEvent(uint32_t windowID, int xPos, int yPos)
		: WindowEvent(windowID), m_xPos(xPos), m_yPos(yPos) {}

private:

	int m_xPos, m_yPos;
};

class WindowMouseLeaveEvent : public WindowEvent
{
public:

	WindowMouseLeaveEvent(uint32_t windowID)
		: WindowEvent(windowID)
	{
	}

};

class WindowMouseEnterEvent : public WindowEvent
{
public:

	WindowMouseEnterEvent(uint32_t windowID)
		: WindowEvent(windowID)
	{
	}

};

}