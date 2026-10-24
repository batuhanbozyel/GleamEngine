#pragma once
#include "Devices/Keyboard.h"
#include "Devices/Mouse.h"
#include "Devices/Gamepad.h"

namespace Gleam {

GSTRUCT(InputControl, "C32EBF98-1C9D-4222-980C-7F96C583C146", Serializable)
{
	GFIELD("2B797634-752E-44E7-8B4D-ED29A2AD1866", Serializable)
	InputDeviceType device = InputDeviceType::Keyboard;

	GFIELD("E3972B3E-3D7E-4B24-A0DE-9CDE6B02A3F8", Serializable)
	uint32_t code = 0;

	bool operator==(const InputControl& other) const
	{
		return device == other.device and code == other.code;
	}

	bool operator!=(const InputControl& other) const
	{
		return not (*this == other);
	}

	static InputControl Make(Key key);

	static InputControl Make(MouseControl control);

	static InputControl Make(GamepadControl control);

	static TArrayView<const uint32_t> GetCodes(InputDeviceType device);

	bool IsValid() const;

	bool IsButton() const;

	TStringView GetName() const;
};

} // namespace Gleam
