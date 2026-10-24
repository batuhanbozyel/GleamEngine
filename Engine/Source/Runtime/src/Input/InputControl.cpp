#include "gpch.h"
#include "InputControl.h"

#include "Serialization/ReflectionUtils.h"

#include <SDL3/SDL_keyboard.h>

#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

using namespace Gleam;

template<typename T>
static TArrayView<const uint32_t> GetEnumCodes()
{
	static const auto codes = []()
	{
		TArray<uint32_t> result;
		for (const auto& enumCase : Reflection::GetEnum<T>().Cases())
		{
			if constexpr (std::is_same_v<T, Key>)
			{
				if (enumCase.Value() == static_cast<int64_t>(Key::None))
				{
					continue;
				}
			}
			result.push_back(static_cast<uint32_t>(enumCase.Value()));
		}
		return result;
	}();
	return TArrayView<const uint32_t>(codes.data(), codes.size());
}

static TStringView GetCaseName(const Reflection::EnumDescription& enumDesc, uint32_t code)
{
	const auto enumCase = ReflectionUtils::FindCase(enumDesc, static_cast<int64_t>(code));
	return enumCase ? ReflectionUtils::ResolveDisplayName(*enumCase) : TStringView("Unknown");
}

InputControl InputControl::Make(Key key)
{
	return InputControl{ .device = InputDeviceType::Keyboard, .code = static_cast<uint32_t>(key) };
}

InputControl InputControl::Make(MouseControl control)
{
	return InputControl{ .device = InputDeviceType::Mouse, .code = static_cast<uint32_t>(control) };
}

InputControl InputControl::Make(GamepadControl control)
{
	return InputControl{ .device = InputDeviceType::Gamepad, .code = static_cast<uint32_t>(control) };
}

TArrayView<const uint32_t> InputControl::GetCodes(InputDeviceType device)
{
	switch (device)
	{
		case InputDeviceType::Keyboard: return GetEnumCodes<Key>();
		case InputDeviceType::Mouse: return GetEnumCodes<MouseControl>();
		case InputDeviceType::Gamepad: return GetEnumCodes<GamepadControl>();
		default: return TArrayView<const uint32_t>();
	}
}

bool InputControl::IsValid() const
{
	const auto codes = GetCodes(device);
	return eastl::find(codes.begin(), codes.end(), code) != codes.end();
}

bool InputControl::IsButton() const
{
	switch (device)
	{
		case InputDeviceType::Keyboard: return true;
		case InputDeviceType::Mouse: return code < Mouse::ButtonCount;
		case InputDeviceType::Gamepad: return code < Gamepad::ButtonCount;
		default: return false;
	}
}

TStringView InputControl::GetName() const
{
	if (IsValid() == false)
	{
		return TStringView("None");
	}

	switch (device)
	{
		case InputDeviceType::Keyboard:
		{
			const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(code));
			return name[0] != 0 ? TStringView(name) : TStringView("Unknown");
		}
		case InputDeviceType::Mouse: return GetCaseName(Reflection::GetEnum<MouseControl>(), code);
		case InputDeviceType::Gamepad: return GetCaseName(Reflection::GetEnum<GamepadControl>(), code);
		default: return TStringView("None");
	}
}
