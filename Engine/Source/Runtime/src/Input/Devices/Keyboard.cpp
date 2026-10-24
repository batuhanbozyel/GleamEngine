#include "gpch.h"
#include "Keyboard.h"

#include <SDL3/SDL_events.h>

using namespace Gleam;

Keyboard::Keyboard(uint32_t id)
	: InputDevice(InputDeviceType::Keyboard, id, "Keyboard")
{

}

bool Keyboard::IsPressed(Key key) const
{
	return mKeys.IsPressed(static_cast<uint32_t>(key));
}

bool Keyboard::WasPressedThisFrame(Key key) const
{
	return mKeys.WasPressedThisFrame(static_cast<uint32_t>(key));
}

bool Keyboard::WasReleasedThisFrame(Key key) const
{
	return mKeys.WasReleasedThisFrame(static_cast<uint32_t>(key));
}

Float3 Keyboard::ReadControl(uint32_t code) const
{
	if (code < KeyCount and mKeys.IsPressed(code))
	{
		return Float3(1.0f, 0.0f, 0.0f);
	}
	return Float3::zero;
}

bool Keyboard::WasControlPressedThisFrame(uint32_t code) const
{
	return code < KeyCount and mKeys.WasPressedThisFrame(code);
}

bool Keyboard::WasControlReleasedThisFrame(uint32_t code) const
{
	return code < KeyCount and mKeys.WasReleasedThisFrame(code);
}

uint64_t Keyboard::GetControlPressTime(uint32_t code) const
{
	return code < KeyCount ? mKeys.GetPressTime(code) : 0;
}

uint64_t Keyboard::GetControlReleaseTime(uint32_t code) const
{
	return code < KeyCount ? mKeys.GetReleaseTime(code) : 0;
}

void Keyboard::Commit()
{
	mKeys.Commit();
}

void Keyboard::ProcessEvent(const SDL_KeyboardEvent& keyboardEvent)
{
	const auto scancode = static_cast<uint32_t>(keyboardEvent.scancode);
	if (scancode >= KeyCount)
	{
		return;
	}

	if (keyboardEvent.down)
	{
		mKeys.Press(scancode, keyboardEvent.timestamp);
	}
	else
	{
		mKeys.Release(scancode, keyboardEvent.timestamp);
	}
}
