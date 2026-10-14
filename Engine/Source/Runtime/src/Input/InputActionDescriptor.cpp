#include "gpch.h"
#include "InputActionDescriptor.h"

#include "Serialization/ReflectionUtils.h"

#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

using namespace Gleam;

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

const char* InputBindingDescriptor::GetPartName(InputBindingType type, uint32_t part)
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
