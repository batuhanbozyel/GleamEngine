#pragma once
#include "Event.h"

namespace Gleam {

class WindowCloseEvent : public Event
{
public:
	WindowCloseEvent() = default;
};

class WindowResizeEvent : public Event
{
public:
	WindowResizeEvent(uint32_t width, uint32_t height)
		: m_Width(width), m_Height(height)
	{
	}

	inline uint32_t GetWidth() const { return m_Width; }
	inline uint32_t GetHeight() const { return m_Height; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "WindowResizeEvent: " << m_Width << ", " << m_Height;
		return ss.str();
	}

private:
	uint32_t m_Width, m_Height;
};

class WindowFocusEvent : public Event
{
public:
	WindowFocusEvent() = default;

};

class WindowLostFocusEvent : public Event
{
public:
	WindowLostFocusEvent() = default;

};

class WindowMovedEvent : public Event
{
public:
	WindowMovedEvent(int xPos, int yPos)
		: m_xPos(xPos), m_yPos(yPos)
	{
	}

private:
	int m_xPos, m_yPos;
};

}