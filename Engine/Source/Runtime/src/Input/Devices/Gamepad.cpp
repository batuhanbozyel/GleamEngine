#include "gpch.h"
#include "Gamepad.h"

#include <SDL3/SDL_events.h>

using namespace Gleam;

static const char* GetGamepadName(SDL_Gamepad* handle)
{
	const char* name = SDL_GetGamepadName(handle);
	return name ? name : "Gamepad";
}

static uint32_t ToButtonIndex(SDL_GamepadButton button)
{
	switch (button)
	{
		case SDL_GAMEPAD_BUTTON_SOUTH: return static_cast<uint32_t>(GamepadControl::South);
		case SDL_GAMEPAD_BUTTON_EAST: return static_cast<uint32_t>(GamepadControl::East);
		case SDL_GAMEPAD_BUTTON_WEST: return static_cast<uint32_t>(GamepadControl::West);
		case SDL_GAMEPAD_BUTTON_NORTH: return static_cast<uint32_t>(GamepadControl::North);
		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return static_cast<uint32_t>(GamepadControl::LeftShoulder);
		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return static_cast<uint32_t>(GamepadControl::RightShoulder);
		case SDL_GAMEPAD_BUTTON_LEFT_STICK: return static_cast<uint32_t>(GamepadControl::LeftStickPress);
		case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return static_cast<uint32_t>(GamepadControl::RightStickPress);
		case SDL_GAMEPAD_BUTTON_DPAD_UP: return static_cast<uint32_t>(GamepadControl::DPadUp);
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return static_cast<uint32_t>(GamepadControl::DPadDown);
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return static_cast<uint32_t>(GamepadControl::DPadLeft);
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return static_cast<uint32_t>(GamepadControl::DPadRight);
		case SDL_GAMEPAD_BUTTON_START: return static_cast<uint32_t>(GamepadControl::Start);
		case SDL_GAMEPAD_BUTTON_BACK: return static_cast<uint32_t>(GamepadControl::Select);
		case SDL_GAMEPAD_BUTTON_GUIDE: return static_cast<uint32_t>(GamepadControl::Guide);
		default: return Gamepad::ButtonCount;
	}
}

static float NormalizeAxis(int16_t value)
{
	return Math::Max(static_cast<float>(value) / static_cast<float>(SDL_JOYSTICK_AXIS_MAX), -1.0f);
}

Gamepad::Gamepad(uint32_t id, SDL_Gamepad* handle)
	: InputDevice(InputDeviceType::Gamepad, id, GetGamepadName(handle))
{
	Connect(handle);
}

bool Gamepad::IsPressed(GamepadControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.IsPressed(index);
}

bool Gamepad::WasPressedThisFrame(GamepadControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.WasPressedThisFrame(index);
}

bool Gamepad::WasReleasedThisFrame(GamepadControl button) const
{
	const auto index = static_cast<uint32_t>(button);
	return index < ButtonCount and mButtons.WasReleasedThisFrame(index);
}

Float2 Gamepad::GetLeftStick() const
{
	return Float2(mAxes[SDL_GAMEPAD_AXIS_LEFTX], mAxes[SDL_GAMEPAD_AXIS_LEFTY]);
}

Float2 Gamepad::GetRightStick() const
{
	return Float2(mAxes[SDL_GAMEPAD_AXIS_RIGHTX], mAxes[SDL_GAMEPAD_AXIS_RIGHTY]);
}

float Gamepad::GetLeftTrigger() const
{
	return mAxes[SDL_GAMEPAD_AXIS_LEFT_TRIGGER];
}

float Gamepad::GetRightTrigger() const
{
	return mAxes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER];
}

void Gamepad::SetRumble(float lowFrequency, float highFrequency, uint32_t durationMs) const
{
	if (mHandle == nullptr)
	{
		return;
	}

	const auto low = static_cast<Uint16>(Math::Saturate(lowFrequency) * 0xFFFF);
	const auto high = static_cast<Uint16>(Math::Saturate(highFrequency) * 0xFFFF);
	SDL_RumbleGamepad(mHandle, low, high, durationMs);
}

Float3 Gamepad::ReadControl(uint32_t code) const
{
	switch (static_cast<GamepadControl>(code))
	{
		case GamepadControl::LeftTrigger: return Float3(GetLeftTrigger(), 0.0f, 0.0f);
		case GamepadControl::RightTrigger: return Float3(GetRightTrigger(), 0.0f, 0.0f);
		case GamepadControl::LeftStick: return Float3(GetLeftStick().x, GetLeftStick().y, 0.0f);
		case GamepadControl::RightStick: return Float3(GetRightStick().x, GetRightStick().y, 0.0f);
		case GamepadControl::LeftStickX: return Float3(mAxes[SDL_GAMEPAD_AXIS_LEFTX], 0.0f, 0.0f);
		case GamepadControl::LeftStickY: return Float3(mAxes[SDL_GAMEPAD_AXIS_LEFTY], 0.0f, 0.0f);
		case GamepadControl::RightStickX: return Float3(mAxes[SDL_GAMEPAD_AXIS_RIGHTX], 0.0f, 0.0f);
		case GamepadControl::RightStickY: return Float3(mAxes[SDL_GAMEPAD_AXIS_RIGHTY], 0.0f, 0.0f);
		default: return Float3(IsPressed(static_cast<GamepadControl>(code)) ? 1.0f : 0.0f, 0.0f, 0.0f);
	}
}

bool Gamepad::WasControlPressedThisFrame(uint32_t code) const
{
	return code < ButtonCount and mButtons.WasPressedThisFrame(code);
}

bool Gamepad::WasControlReleasedThisFrame(uint32_t code) const
{
	return code < ButtonCount and mButtons.WasReleasedThisFrame(code);
}

void Gamepad::Commit()
{
	mButtons.Commit();
	mAxes = mPendingAxes;
}

void Gamepad::Connect(SDL_Gamepad* handle)
{
	mHandle = handle;
	mJoystickId = SDL_GetGamepadID(handle);
	mGuid = SDL_GetGamepadGUIDForID(mJoystickId);
	mConnected = true;
}

void Gamepad::Disconnect()
{
	mButtons.ReleaseAll();
	mPendingAxes.fill(0.0f);
	SDL_CloseGamepad(mHandle);
	mHandle = nullptr;
	mJoystickId = 0;
	mConnected = false;
}

bool Gamepad::Matches(const SDL_GUID& guid) const
{
	return SDL_memcmp(mGuid.data, guid.data, sizeof(guid.data)) == 0;
}

SDL_JoystickID Gamepad::GetJoystickId() const
{
	return mJoystickId;
}

void Gamepad::ProcessButtonEvent(const SDL_GamepadButtonEvent& buttonEvent)
{
	const auto index = ToButtonIndex(static_cast<SDL_GamepadButton>(buttonEvent.button));
	if (index >= ButtonCount)
	{
		return;
	}

	if (buttonEvent.down)
	{
		mButtons.Press(index);
	}
	else
	{
		mButtons.Release(index);
	}
}

void Gamepad::ProcessAxisEvent(const SDL_GamepadAxisEvent& axisEvent)
{
	if (axisEvent.axis >= SDL_GAMEPAD_AXIS_COUNT)
	{
		return;
	}

	auto value = NormalizeAxis(axisEvent.value);
	switch (axisEvent.axis)
	{
		case SDL_GAMEPAD_AXIS_LEFTY:
		case SDL_GAMEPAD_AXIS_RIGHTY:
		{
			value = -value;
			break;
		}
		case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
		{
			UpdateTriggerButton(GamepadControl::LeftTrigger, value);
			break;
		}
		case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
		{
			UpdateTriggerButton(GamepadControl::RightTrigger, value);
			break;
		}
		default: break;
	}
	mPendingAxes[axisEvent.axis] = value;
}

void Gamepad::UpdateTriggerButton(GamepadControl trigger, float value)
{
	const auto index = static_cast<uint32_t>(trigger);
	if (value >= TriggerPressPoint)
	{
		mButtons.Press(index);
	}
	else
	{
		mButtons.Release(index);
	}
}
