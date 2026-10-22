#include "gpch.h"
#include "InputActionDescriptor.h"

#include "Serialization/ReflectionUtils.h"

#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

using namespace Gleam;

InputBindingDescriptor InputBindingDescriptor::MakeKeyBinding(Key key, EnumFlag<ModifierKeys> modifiers)
{
	InputBindingDescriptor binding;
	binding.type = InputBindingType::Control;
	binding.controls.push_back(InputControl::Make(key));
	binding.modifiers = modifiers;
	binding.layoutMapped = true;
	return binding;
}

InputBindingDescriptor InputBindingDescriptor::MakeMouseBinding(MouseControl control, EnumFlag<ModifierKeys> modifiers)
{
	InputBindingDescriptor binding;
	binding.type = InputBindingType::Control;
	binding.controls.push_back(InputControl::Make(control));
	binding.modifiers = modifiers;
	return binding;
}

InputBindingDescriptor InputBindingDescriptor::MakeAxisBinding(InputBindingType type, std::initializer_list<Key> keys)
{
	InputBindingDescriptor binding;
	binding.type = type;
	for (const auto key : keys)
	{
		binding.controls.push_back(InputControl::Make(key));
	}
	return binding;
}

uint32_t InputBindingDescriptor::GetPartCount(InputBindingType type)
{
	switch (type)
	{
		case InputBindingType::Control: return 1;
		case InputBindingType::Axis1D: return 2;
		case InputBindingType::Axis2D: return 4;
		case InputBindingType::Axis3D: return 6;
		default: return 0;
	}
}

const TStringView InputBindingDescriptor::GetPartName(InputBindingType type, uint32_t part)
{
	static constexpr const char* kAxis1DParts[] = { "Negative", "Positive" };
	static constexpr const char* kAxisParts[] = { "Up", "Down", "Left", "Right", "Forward", "Backward" };

	if (part >= GetPartCount(type))
	{
		return "";
	}

	switch (type)
	{
		case InputBindingType::Control: return "Control";
		case InputBindingType::Axis1D: return kAxis1DParts[part];
		default: return kAxisParts[part];
	}
}

TString InputBindingDescriptor::GetDisplayName() const
{
	TString text;
	ReflectionUtils::ForEachSetCase(modifiers.Value(), Reflection::GetEnum<ModifierKeys>(), [&text](const Reflection::EnumCaseDescription& enumCase)
	{
		const auto name = ReflectionUtils::ResolveDisplayName(enumCase);
		text.append(name.data(), name.size());
		text.append("+");
	});

	for (uint32_t part = 0; part < controls.size(); ++part)
	{
		if (part > 0)
		{
			text.append(" / ");
		}
		const auto name = controls[part].GetName();
		text.append(name.data(), name.size());
	}
	return text;
}

InputActionDescriptor InputActionDescriptor::Make(const TStringView name, InputValueType valueType, std::initializer_list<InputBindingDescriptor> bindings)
{
	InputActionDescriptor action;
	action.id = Guid::NewGuid();
	action.name = name;
	action.valueType = valueType;
	action.bindings = bindings;
	return action;
}

InputActionMapDescriptor InputActionMapDescriptor::Make(const TStringView name, int32_t priority, std::initializer_list<InputActionDescriptor> actions)
{
	InputActionMapDescriptor map;
	map.id = Guid::NewGuid();
	map.name = name;
	map.priority = priority;
	map.actions = actions;
	return map;
}

InputActionAssetDescriptor InputActionAssetDescriptor::Make(std::initializer_list<InputActionMapDescriptor> maps)
{
	InputActionAssetDescriptor actions;
	actions.maps = maps;
	return actions;
}