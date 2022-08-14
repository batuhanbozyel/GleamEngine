#pragma once
#include "Event.h"
#include "Input/KeyCode.h"

namespace Gleam {

class KeyEvent : public Event
{
public:

	KeyCode GetKeyCode() const { return mKeyCode; }

protected:
	KeyEvent(KeyCode keyCode)
		: mKeyCode(keyCode) {}

    KeyCode mKeyCode;
};

class KeyPressedEvent : public KeyEvent
{
public:

	KeyPressedEvent(KeyCode keyCode, uint8_t repeatCount)
		: KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

    uint8_t GetRepeatCount() const { return m_RepeatCount; }

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyPressedEvent: " << char(mKeyCode);
		return ss.str();
	}

private:

    uint8_t m_RepeatCount;
};

class KeyReleasedEvent : public KeyEvent
{
public:

	KeyReleasedEvent(KeyCode keyCode)
		: KeyEvent(keyCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyReleasedEvent: " << char(mKeyCode);
		return ss.str();
	}
};

class KeyTypedEvent : public KeyEvent
{
public:

	KeyTypedEvent(KeyCode keyCode)
		: KeyEvent(keyCode) {}

	TString ToString() const override
	{
		TStringStream ss;
		ss << "KeyTypedEvent: " << char(mKeyCode);
		return ss.str();
	}
};

} // namespace Gleam
