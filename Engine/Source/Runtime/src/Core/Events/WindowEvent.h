#pragma once
#include <SDL.h>
#include "Event.h"

namespace Gleam {

class WindowEvent : public Event
{
public:

	WindowEvent(SDL_Window* window)
		: m_Window(window) {}

	SDL_Window* GetWindow() const
	{
		return m_Window;
	}

private:

	SDL_Window* m_Window;
};

class WindowCloseEvent : public WindowEvent
{
public:
	WindowCloseEvent(SDL_Window* window)
		: WindowEvent(window) {}

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

	WindowResizeEvent(SDL_Window* window, uint32_t width, uint32_t height)
		: WindowEvent(window), m_Width(width), m_Height(height) {}

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

	WindowMaximizeEvent(SDL_Window* window)
		: WindowEvent(window) {}

};

class WindowMinimizeEvent : public WindowEvent
{
public:

	WindowMinimizeEvent(SDL_Window* window)
		: WindowEvent(window) {}

};

class WindowRestoreEvent : public WindowEvent
{
public:

	WindowRestoreEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

};

class WindowFocusEvent : public WindowEvent
{
public:

	WindowFocusEvent(SDL_Window* window)
		: WindowEvent(window) {}

};

class WindowLostFocusEvent : public WindowEvent
{
public:

	WindowLostFocusEvent(SDL_Window* window)
		: WindowEvent(window) {}

};

class WindowMovedEvent : public WindowEvent
{
public:

	WindowMovedEvent(SDL_Window* window, int xPos, int yPos)
		: WindowEvent(window), m_xPos(xPos), m_yPos(yPos) {}

private:

	int m_xPos, m_yPos;
};

class WindowMouseLeaveEvent : public WindowEvent
{
public:

	WindowMouseLeaveEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

};

class WindowMouseEnterEvent : public WindowEvent
{
public:

	WindowMouseEnterEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

};

}