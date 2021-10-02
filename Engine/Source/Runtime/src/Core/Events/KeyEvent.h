#pragma once
#include "Event.h"

namespace Gleam {

class KeyEvent : public Event
{
public:

	int GetKeyCode() const { return m_KeyCode; }

protected:
	KeyEvent(int keyCode)
		: m_KeyCode(keyCode) {}

	int m_KeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:

	KeyPressedEvent(int keyCode, int repeatCount)
		: KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

	int GetRepeatCount() const { return m_RepeatCount; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyPressedEvent: " << m_KeyCode;
		return ss.str();
	}

private:

	int m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:

	KeyReleasedEvent(int keyCode)
		: KeyEvent(keyCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyReleasedEvent: " << m_KeyCode;
		return ss.str();
	}
};

class KeyTypedEvent : public KeyEvent
{
public:

	KeyTypedEvent(int keyCode)
		: KeyEvent(keyCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyTypedEvent: " << m_KeyCode;
		return ss.str();
	}
};

}