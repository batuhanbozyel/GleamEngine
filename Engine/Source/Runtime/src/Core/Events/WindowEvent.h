#pragma once
#include "Event.h"
#include "Math/Vector2.h"

struct SDL_Window;

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

	virtual TString ToString() const override
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

	virtual TString ToString() const override
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

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowMaximizeEvent";
		return ss.str();
	}

};

class WindowMinimizeEvent : public WindowEvent
{
public:

	WindowMinimizeEvent(SDL_Window* window)
		: WindowEvent(window) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowMinimizeEvent";
		return ss.str();
	}

};

class WindowRestoreEvent : public WindowEvent
{
public:

	WindowRestoreEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowRestoreEvent";
		return ss.str();
	}

};

class WindowFocusEvent : public WindowEvent
{
public:

	WindowFocusEvent(SDL_Window* window)
		: WindowEvent(window) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowFocusEvent";
		return ss.str();
	}

};

class WindowLostFocusEvent : public WindowEvent
{
public:

	WindowLostFocusEvent(SDL_Window* window)
		: WindowEvent(window) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowLostFocusEvent";
		return ss.str();
	}

};

class WindowMovedEvent : public WindowEvent
{
public:

	WindowMovedEvent(SDL_Window* window, int xPos, int yPos)
		: WindowEvent(window), mWindowPos(xPos, yPos) {}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowMovedEvent: " << mWindowPos.x << ", " << mWindowPos.y;
		return ss.str();
	}

private:

	Int2 mWindowPos;
};

class WindowMouseLeaveEvent : public WindowEvent
{
public:

	WindowMouseLeaveEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowMouseLeaveEvent";
		return ss.str();
	}

};

class WindowMouseEnterEvent : public WindowEvent
{
public:

	WindowMouseEnterEvent(SDL_Window* window)
		: WindowEvent(window)
	{
	}

	virtual TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowMouseEnterEvent";
		return ss.str();
	}

};

} // namespace Gleam
