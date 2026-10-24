#pragma once
#include "InputControl.h"

#include "Core/Macro.h"
#include "Core/PlatformTargetDefines.h"
#include "Container/EnumFlag.h"

namespace Gleam {

GENUM(InputValueType, "04FE15A8-7B76-49C3-BECF-AB9999FC709E", Serializable, PrettyName("Value Type"))
{
	GITEM(Bool, "D848BCB4-F309-4251-9C13-BEA088BF945F"),
	GITEM(Float, "B042787B-57DB-4362-AA98-D24964E9F0E5"),
	GITEM(Float2, "3CA25369-4992-493C-9A57-D43F9A95FBF0"),
	GITEM(Float3, "57FE0235-B120-436B-AD4E-F1E29518AD1A")
};

GENUM(InputBindingType, "83B4F15E-71C4-4FEE-B120-DDBEBC18977D", Serializable, PrettyName("Binding Type"))
{
	GITEM(Control, "88E2210A-603E-42B1-B4E7-5CE83477D614"),
	GITEM(Axis1D, "1920B57B-C705-4A41-BF0C-B628E0F20315"),
	GITEM(Axis2D, "E6121CC3-C5A0-4D74-8A3D-A1983887E854"),
	GITEM(Axis3D, "A5806BB3-9028-4FD7-A480-C1169CD8E3D8")
};

GENUM(ModifierKeys, "95DF53D1-C9A3-4F71-B0C8-39689B0A41C5", Serializable, PrettyName("Modifiers")) : uint32_t
{
	GITEM(Shift, "37D2C80E-36A2-4F7B-AD69-CDD6BD187661") = BIT(0),
	GITEM(Control, "A48BD063-E2C6-4481-92B1-C7A827CF4AEC", PrettyName("Ctrl")) = BIT(1),
#if defined(PLATFORM_MACOS)
	GITEM(Alt, "3DAF25D6-CCC4-4DAC-AC69-7DB19BE4F950", PrettyName("Option")) = BIT(2),
	GITEM(Super, "9FC903FE-56A2-461B-8C02-4020EFF2A5B6", PrettyName("Cmd")) = BIT(3),
#else
	GITEM(Alt, "3DAF25D6-CCC4-4DAC-AC69-7DB19BE4F950") = BIT(2),
	GITEM(Super, "9FC903FE-56A2-461B-8C02-4020EFF2A5B6") = BIT(3),
#endif
	GITEM(Shortcut, "EF3C337E-94D8-47CD-8CA3-004765116E2C") = BIT(4)
};

GENUM(InputProcessorType, "7F4699BE-7CA3-4A51-BAFA-0F039A4E0AA2", Serializable, PrettyName("Processor"))
{
	GITEM(DeadZone, "2C6E229E-ED35-4C08-9F7A-0DA514F7637E"),
	GITEM(Scale, "B4CE95B4-1CB7-4ADC-A6DA-08A57C14FDF0"),
	GITEM(Invert, "5C751836-197B-4F93-AD37-CF5E944DBA61"),
	GITEM(Normalize, "240936C6-7E39-4BFA-93A0-B01B715F6BD7"),
	GITEM(Clamp, "AFF7C633-4CF2-4419-9B2D-1F4536431C61")
};

GENUM(InputInteractionType, "7D061C73-0158-4B4B-A669-70302D8F4D2B", Serializable, PrettyName("Interaction"))
{
	GITEM(Press, "7BB706F6-F8AF-49E6-B0DA-29A6D8440822"),
	GITEM(Release, "921E1612-85B3-4FA8-B1C7-9905D2D24CDB"),
	GITEM(Hold, "600D4651-4D23-4A22-915B-ABDA542673E2"),
	GITEM(Tap, "B5401AAB-EEF3-4A60-BD72-A545388F691C"),
	GITEM(MultiTap, "03FA2ED6-DABF-4814-BC6B-A07792AABDB9")
};

struct InputActionDescriptor;

GSTRUCT(InputProcessor,"3AEFE2EB-5F9E-4E1D-B1A7-1AD91D34E4EF", Serializable)
{
	GFIELD("0AA3414C-8837-4F8B-81F0-DF950531ECE8", Serializable)
	InputProcessorType type = InputProcessorType::DeadZone;

	GFIELD("366B9653-73BE-42AF-94DF-946D3C513DAF", Serializable)
	float min = 0.125f;

	GFIELD("6480227D-C326-4D41-B112-A2EC29886584", Serializable)
	float max = 0.925f;

	GFIELD("EA38FB59-5DB4-4510-A608-C8AF11A2743F", Serializable)
	Float3 factor = Float3::one;

	Float3 Apply(const Float3& value, const InputActionDescriptor& action) const;
};

GSTRUCT(InputInteraction, "D2F1CF61-9693-4725-BF48-6DDDE726B6BE", Serializable)
{
	GFIELD("A83745CA-EDB4-4A87-BEBB-124A3EA846BD", Serializable)
	InputInteractionType type = InputInteractionType::Press;

	GFIELD("ADBA2912-9450-4B9A-B167-65C76EE94154", Serializable)
	float duration = 0.2f;

	GFIELD("4BD2B966-E887-440B-8591-14983B1B8DDD", Serializable)
	float pressPoint = 0.5f;

	GFIELD("9A0165AF-5DE3-410F-975F-8FD8E163A691", Serializable)
	uint32_t tapCount = 2;
};

GSTRUCT(InputBindingDescriptor, "F2CB4777-5EB9-47F8-93B3-42A69351A40C", Serializable)
{
	GFIELD("5F31D758-C01E-42D8-8F78-96425C88CB4C", Serializable)
	Guid id;

	GFIELD("94F4D731-469C-4A24-8162-A1DC7E784E4A", Serializable)
	InputBindingType type = InputBindingType::Control;

	GFIELD("E5F7C3BC-A45E-4EFB-89E0-6B654670A4BD", Serializable)
	TArray<InputControl> controls;

	GFIELD("C4065558-4003-45EF-9941-DEDE57025BDC", Serializable)
	EnumFlag<ModifierKeys> modifiers;

	GFIELD("583E1399-4AF5-4DCB-97FB-B37425DC8618", Serializable)
	TArray<InputProcessor> processors;

	GFIELD("34D8C52F-21A5-4FF8-94F9-0ED34EBFE041", Serializable)
	TArray<InputInteraction> interactions;

	TString GetDisplayName() const;

	static uint32_t GetPartCount(InputBindingType type);

	static const TStringView GetPartName(InputBindingType type, uint32_t part);

	static InputBindingDescriptor MakeKeyBinding(Key key, EnumFlag<ModifierKeys> modifiers = {});

	static InputBindingDescriptor MakeMouseBinding(MouseControl control, EnumFlag<ModifierKeys> modifiers = {});

	static InputBindingDescriptor MakeAxisBinding(InputBindingType type, std::initializer_list<Key> keys);
};

GSTRUCT(InputActionDescriptor, "40E35849-CB40-4E3C-87CE-4048B3FF66B7", Serializable)
{
	GFIELD("24F59B80-F5D1-4F1C-BEA9-4F0BC16A1B88", Serializable)
	Guid id;

	GFIELD("F2AA5191-E56F-4E27-B3BC-BC9B6E97CCE0", Serializable)
	TString name;

	GFIELD("F8DEF438-E969-4E51-8BCB-0458DE95A47E", Serializable)
	InputValueType valueType = InputValueType::Bool;

	GFIELD("ED895489-731C-477D-9EBE-BF3401A746E0", Serializable)
	TArray<InputBindingDescriptor> bindings;

	GFIELD("76191E41-9A6E-401C-AA55-5B2EFEFF6A9D", Serializable)
	TArray<InputProcessor> processors;

	GFIELD("7E67BB54-315D-4EC4-8E74-ECEA161C2CD1", Serializable)
	TArray<InputInteraction> interactions;

	float ComputeMagnitude(const Float3& value) const;

	static InputActionDescriptor Make(const TStringView name, InputValueType valueType, std::initializer_list<InputBindingDescriptor> bindings);
};

GSTRUCT(InputActionMapDescriptor, "3BBE3711-B6B3-489D-83F4-AEF36968212E", Serializable)
{
	GFIELD("1AE4F5EC-E1DC-4E74-A11D-F88F800CE86B", Serializable)
	Guid id;

	GFIELD("E0237A01-8B39-42F8-A003-B6218C640FC4", Serializable)
	TString name;

	GFIELD("55EBA1E9-041E-4339-8E6C-78D14690B02B", Serializable)
	int32_t priority = 0;

	GFIELD("7A745B0A-4C9A-4384-9DE0-574ADA47BC9D", Serializable)
	TArray<InputActionDescriptor> actions;

	static InputActionMapDescriptor Make(const TStringView name, int32_t priority, std::initializer_list<InputActionDescriptor> actions);
};

GSTRUCT(InputActionAssetDescriptor, "4CAD0553-93E9-499A-97A5-4113451B9272", Serializable)
{
	GFIELD("657A6F2C-5545-4D29-807D-D9889B220587", Serializable)
	TArray<InputActionMapDescriptor> maps;

	static InputActionAssetDescriptor Make(std::initializer_list<InputActionMapDescriptor> maps);
};

GSTRUCT(InputBindingOverride, "9DB685A2-FFD6-47BA-B39B-07415EFF4BF2", Serializable)
{
	GFIELD("BDBBD13C-33E2-4604-9D91-A6CA59C02A5F", Serializable)
	Guid map;

	GFIELD("1D6B8272-CE71-48D4-B01E-E31B68A55D03", Serializable)
	Guid action;

	GFIELD("AD2B000D-7163-44A7-81B3-078AD97F8705", Serializable)
	Guid binding;

	GFIELD("C696DAF4-373F-45A7-857F-604C3E1A0656", Serializable)
	uint32_t part = 0;

	GFIELD("AECA6180-584B-4237-879B-0C04B8B17CAB", Serializable)
	InputControl control;
};

GSTRUCT(InputBindingOverrides, "090B8569-BA22-4CDF-A743-4185EC4DEBAD", Serializable)
{
	GFIELD("21EB7C70-6AB7-4FF9-9DEF-70AAFF9A5A4E", Serializable)
	TArray<InputBindingOverride> overrides;
};

} // namespace Gleam
