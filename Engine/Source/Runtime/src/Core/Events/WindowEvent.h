#pragma once
#include <SDL3/SDL.h>
#include "Event.h"

namespace Gleam {

class WindowEvent : public Event
{
public:

	WindowEvent(SDL_Window* window)
		: mWindow(window) {}

	SDL_Window* GetWindow() const
	{
		return mWindow;
	}

private:

	SDL_Window* mWindow;
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
		: WindowEvent(window), mWidth(width), mHeight(height) {}

	uint32_t GetWidth() const { return mWidth; }
	uint32_t GetHeight() const { return mHeight; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowResizeEvent: " << mWidth << ", " << mHeight;
		return ss.str();
	}

private:

	uint32_t mWidth, mHeight;
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
		: WindowEvent(window), mWindowPos(xPos, yPos) {}

private:

	Vector2Int mWindowPos;
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

} // namespace Gleam
