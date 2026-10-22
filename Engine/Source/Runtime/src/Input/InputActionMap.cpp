#include "gpch.h"
#include "InputActionMap.h"
#include "InputUser.h"
#include "InputSystem.h"

#include <EASTL/sort.h>

using namespace Gleam;

static uint64_t ConsumptionKey(const InputDevice* device, uint32_t code)
{
	return (static_cast<uint64_t>(device->GetId()) << 32) | code;
}

static uint32_t CountModifiers(EnumFlag<ModifierKeys> modifiers)
{
	uint32_t count = 0;
	for (auto mask = modifiers.Value(); mask != 0; mask &= mask - 1)
	{
		++count;
	}
	return count;
}

static bool IsModifierHeld(const Keyboard* keyboard, ModifierKeys modifier)
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

void InputConsumption::BeginMap(const Guid& map)
{
	mMap = map;
	mMapControls.clear();
}

bool InputConsumption::IsConsumed(const InputDevice* device, uint32_t code) const
{
	const auto key = ConsumptionKey(device, code);
	const auto it = mControls.find(key);
	return mMapControls.find(key) != mMapControls.end() or (it != mControls.end() and it->second != mMap);
}

void InputConsumption::Consume(const InputDevice* device, uint32_t code)
{
	const auto key = ConsumptionKey(device, code);
	mMapControls.insert(key);
	mControls.insert(eastl::make_pair(key, mMap));
}

InputActionMap::InputActionMap(const InputActionMapDescriptor& descriptor)
	: mId(descriptor.id)
	, mName(descriptor.name)
	, mPriority(descriptor.priority)
{
	mActions.reserve(descriptor.actions.size());
	for (const auto& action : descriptor.actions)
	{
		mActions.push_back(CreateScope<InputAction>(this, action));
	}
	BuildBindingOrder();
}

const Guid& InputActionMap::GetId() const
{
	return mId;
}

const TString& InputActionMap::GetName() const
{
	return mName;
}

int32_t InputActionMap::GetPriority() const
{
	return mPriority;
}

bool InputActionMap::IsEnabled() const
{
	return mEnabled;
}

void InputActionMap::SetEnabled(bool enabled)
{
	mEnabled = enabled;
	RefreshActivation();
}

InputAction* InputActionMap::FindAction(TStringView name) const
{
	for (const auto& action : mActions)
	{
		if (action->GetName() == name)
		{
			return action.get();
		}
	}
	return nullptr;
}

const TArray<Scope<InputAction>>& InputActionMap::GetActions() const
{
	return mActions;
}

bool InputActionMap::IsActive() const
{
	return mActive;
}

void InputActionMap::SetUserActive(bool active)
{
	mUserActive = active;
	RefreshActivation();
}

void InputActionMap::RefreshActivation()
{
	const bool active = mEnabled and mUserActive;
	if (active == mActive)
	{
		return;
	}

	mActive = active;
	const double time = InputSystem::GetTime();
	for (auto& action : mActions)
	{
		if (active)
		{
			action->Enable();
		}
		else
		{
			action->Disable(time);
		}
	}
}

void InputActionMap::BuildBindingOrder()
{
	mBindingOrder.clear();
	for (uint32_t actionIndex = 0; actionIndex < mActions.size(); ++actionIndex)
	{
		const auto& bindings = mActions[actionIndex]->GetBindings();
		for (uint32_t bindingIndex = 0; bindingIndex < bindings.size(); ++bindingIndex)
		{
			mBindingOrder.push_back(BindingEntry{
				.action = actionIndex,
				.binding = bindingIndex,
				.modifierCount = CountModifiers(bindings[bindingIndex].modifiers)
			});
		}
	}

	eastl::stable_sort(mBindingOrder.begin(), mBindingOrder.end(), [](const BindingEntry& lhs, const BindingEntry& rhs)
	{
		return lhs.modifierCount > rhs.modifierCount;
	});
}

void InputActionMap::ResolveBindings()
{
	for (auto& action : mActions)
	{
		action->ResolveBindings();
	}
}

void InputActionMap::SyncBindingOverrides(const InputUser& user)
{
	if (mOverridesVersion != user.GetBindingOverridesVersion())
	{
		mOverridesVersion = user.GetBindingOverridesVersion();
		for (auto& action : mActions)
		{
			action->ResetBindings();
		}

		for (const auto& entry : user.GetBindingOverrides().overrides)
		{
			if (entry.map != mId)
			{
				continue;
			}

			for (auto& action : mActions)
			{
				if (action->GetId() == entry.action)
				{
					action->ApplyOverride(entry.binding, entry.part, entry.control);
				}
			}
		}
	}
}

void InputActionMap::Update(const InputUser& user, InputConsumption& consumption, double time)
{
	const auto& devices = user.GetPairedDevices();
	consumption.BeginMap(mId);

	for (auto& action : mActions)
	{
		action->BeginFrame();
	}

	for (const auto& entry : mBindingOrder)
	{
		auto& action = *mActions[entry.action];
		if (entry.binding >= action.mResolvedBindings.size())
		{
			continue;
		}

		auto state = EvaluateBinding(action.mResolvedBindings[entry.binding], action.GetValueType(), devices, consumption);
		state.binding = entry.binding;
		action.Accumulate(state);
	}

	for (auto& action : mActions)
	{
		action->Process(time);
	}
}

InputBindingState InputActionMap::EvaluateBinding(const InputBindingDescriptor& binding, InputValueType valueType, const TArray<InputDevice*>& devices, InputConsumption& consumption) const
{
	InputBindingState state;
	const uint32_t partCount = InputBindingDescriptor::GetPartCount(binding.type);
	if (partCount == 0 or binding.controls.size() < partCount or ModifiersHeld(binding.modifiers, devices) == false)
	{
		return state;
	}

	TFixedArray<ControlSample, 6> samples;
	for (uint32_t i = 0; i < partCount; ++i)
	{
		samples.push_back(SampleControl(binding.controls[i], devices, consumption));
	}

	switch (binding.type)
	{
		case InputBindingType::Control:
		{
			state.value = samples[0].value;
			break;
		}
		case InputBindingType::Axis1D:
		{
			state.value = Float3(samples[1].magnitude - samples[0].magnitude, 0.0f, 0.0f);
			break;
		}
		case InputBindingType::Axis2D:
		{
			state.value = Float3(samples[3].magnitude - samples[2].magnitude, samples[0].magnitude - samples[1].magnitude, 0.0f);
			break;
		}
		case InputBindingType::Axis3D:
		{
			state.value = Float3(samples[3].magnitude - samples[2].magnitude, samples[0].magnitude - samples[1].magnitude, samples[4].magnitude - samples[5].magnitude);
			break;
		}
		default: break;
	}

	float strongest = 0.0f;
	for (const auto& sample : samples)
	{
		state.pressed |= sample.pressed;
		state.released |= sample.released;
		if (sample.device and (state.device == nullptr or sample.magnitude > strongest))
		{
			state.device = sample.device;
			strongest = sample.magnitude;
		}
	}

	for (const auto& processor : binding.processors)
	{
		state.value = InputAction::ApplyProcessor(processor, state.value, valueType);
	}
	state.magnitude = InputAction::ComputeMagnitude(state.value, valueType);

	if (state.magnitude > 0.0f or state.pressed)
	{
		for (uint32_t i = 0; i < partCount; ++i)
		{
			if (samples[i].magnitude > 0.0f or samples[i].pressed)
			{
				ConsumeControl(binding.controls[i], devices, consumption);
			}
		}
	}
	return state;
}

InputActionMap::ControlSample InputActionMap::SampleControl(const InputControl& control, const TArray<InputDevice*>& devices, const InputConsumption& consumption) const
{
	ControlSample sample;
	for (const auto device : devices)
	{
		if (device->GetType() != control.device or device->IsConnected() == false or consumption.IsConsumed(device, control.code))
		{
			continue;
		}

		const auto value = device->ReadControl(control.code);
		const float magnitude = Math::Length(value);
		const bool pressed = device->WasControlPressedThisFrame(control.code);
		const bool released = device->WasControlReleasedThisFrame(control.code);
		if (magnitude > sample.magnitude or (sample.device == nullptr and (pressed or released)))
		{
			sample.value = value;
			sample.magnitude = magnitude;
			sample.device = device;
		}
		sample.pressed |= pressed;
		sample.released |= released;
	}
	return sample;
}

void InputActionMap::ConsumeControl(const InputControl& control, const TArray<InputDevice*>& devices, InputConsumption& consumption) const
{
	for (const auto device : devices)
	{
		if (device->GetType() == control.device)
		{
			consumption.Consume(device, control.code);
		}
	}
}

bool InputActionMap::ModifiersHeld(EnumFlag<ModifierKeys> modifiers, const TArray<InputDevice*>& devices) const
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
