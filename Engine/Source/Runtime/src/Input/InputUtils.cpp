#include "gpch.h"
#include "InputUtils.h"
#include <SDL3/SDL.h>

using namespace Gleam;

static constexpr float kActuationThreshold = 0.5f;

bool InputUtils::FindActuatedControl(TArrayView<InputDevice* const> devices, InputControl& control)
{
	for (const auto device : devices)
	{
		if (device->IsConnected() == false)
		{
			continue;
		}

		for (const auto code : InputControl::GetCodes(device->GetType()))
		{
			const auto candidate = InputControl{ .device = device->GetType(), .code = code };
			if (candidate.IsButton())
			{
				if (device->WasControlPressedThisFrame(code))
				{
					control = candidate;
					return true;
				}
			}
			else if (candidate.device == InputDeviceType::Gamepad)
			{
				const auto stick = static_cast<GamepadControl>(code);
				if ((stick == GamepadControl::LeftStick or stick == GamepadControl::RightStick) and Math::Length(device->ReadControl(code)) >= kActuationThreshold)
				{
					control = candidate;
					return true;
				}
			}
		}
	}
	return false;
}

bool InputUtils::IsModifierHeld(const Keyboard* keyboard, ModifierKeys modifier)
{
	switch (modifier)
	{
		case ModifierKeys::Shift: return keyboard->IsPressed(Key::LeftShift) or keyboard->IsPressed(Key::RightShift);
		case ModifierKeys::Control: return keyboard->IsPressed(Key::LeftControl) or keyboard->IsPressed(Key::RightControl);
		case ModifierKeys::Alt: return keyboard->IsPressed(Key::LeftAlt) or keyboard->IsPressed(Key::RightAlt);
		case ModifierKeys::Super: return keyboard->IsPressed(Key::LeftSuper) or keyboard->IsPressed(Key::RightSuper);
#if defined(PLATFORM_MACOS)
		case ModifierKeys::Shortcut: return IsModifierHeld(keyboard, ModifierKeys::Super);
#else
		case ModifierKeys::Shortcut: return IsModifierHeld(keyboard, ModifierKeys::Control);
#endif
		default: return false;
	}
}

bool InputUtils::ModifiersHeld(EnumFlag<ModifierKeys> modifiers, TArrayView<InputDevice* const> devices)
{
	if (modifiers.Empty())
	{
		return true;
	}

	for (const auto device : devices)
	{
		if (device->GetType() != InputDeviceType::Keyboard)
		{
			continue;
		}

		const auto keyboard = static_cast<const Keyboard*>(device);
		bool held = true;
		for (auto mask = modifiers.Value(); mask != 0 and held; mask &= mask - 1)
		{
			held = IsModifierHeld(keyboard, static_cast<ModifierKeys>(mask & (~mask + 1)));
		}

		if (held)
		{
			return true;
		}
	}
	return false;
}

uint32_t InputUtils::ResolveLayoutKey(uint32_t code)
{
	if (code >= SDL_SCANCODE_A and code <= SDL_SCANCODE_Z)
	{
		const auto key = static_cast<SDL_Keycode>(SDLK_A + (code - SDL_SCANCODE_A));
		const auto scancode = SDL_GetScancodeFromKey(key, nullptr);
		if (scancode != SDL_SCANCODE_UNKNOWN)
		{
			return static_cast<uint32_t>(scancode);
		}
	}
	return code;
}
