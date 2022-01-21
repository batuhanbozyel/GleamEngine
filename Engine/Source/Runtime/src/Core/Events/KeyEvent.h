#pragma once
#include "Event.h"

namespace Gleam {

class KeyEvent : public Event
{
public:

	int GetKeyCode() const { return mKeyCode; }

protected:
	KeyEvent(int keyCode)
		: mKeyCode(keyCode) {}

	int mKeyCode;
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
		ss << "KeyPressedEvent: " << mKeyCode;
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
		ss << "KeyReleasedEvent: " << mKeyCode;
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
		ss << "KeyTypedEvent: " << mKeyCode;
		return ss.str();
	}
};

} // namespace Gleam