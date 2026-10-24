#pragma once
#include "Devices/Keyboard.h"
#include "InputActionDescriptor.h"

namespace Gleam::InputUtils {

bool FindActuatedControl(TArrayView<InputDevice* const> devices, InputControl& control);

bool IsModifierHeld(const Keyboard* keyboard, ModifierKeys modifier);

bool ModifiersHeld(EnumFlag<ModifierKeys> modifiers, TArrayView<InputDevice* const> devices);

uint32_t ResolveLayoutKey(uint32_t code);

} // namespace Gleam::InputUtils
