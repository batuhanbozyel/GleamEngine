#pragma once
#include "InputAction.h"

#include "Container/Hash.h"
#include "Container/Pointer.h"

namespace Gleam {

class PlayerInput;

class InputConsumption
{
public:

	bool IsConsumed(const InputDevice* device, uint32_t code) const;

	void Consume(const InputDevice* device, uint32_t code);

private:

	HashSet<uint64_t> mControls;

};

class InputActionMap
{
	friend class PlayerInput;
	friend class InputSystem;
public:

	InputActionMap(PlayerInput* player, const InputActionMapDescriptor& descriptor, int32_t priority);

	const Guid& GetId() const;

	const TString& GetName() const;

	int32_t GetPriority() const;

	PlayerInput* GetPlayer() const;

	bool IsEnabled() const;

	void SetEnabled(bool enabled);

	InputAction* FindAction(TStringView name) const;

	const TArray<Scope<InputAction>>& GetActions() const;

private:

	struct BindingEntry
	{
		uint32_t action = 0;
		uint32_t binding = 0;
		uint32_t modifierCount = 0;
	};

	struct ControlSample
	{
		Float3 value = Float3::zero;
		float magnitude = 0.0f;
		bool pressed = false;
		bool released = false;
		const InputDevice* device = nullptr;
	};

	bool IsActive() const;

	void RefreshActivation();

	void BuildBindingOrder();

	void ResolveBindings();

	void Update(InputConsumption& consumption, double time);

	InputBindingState EvaluateBinding(const InputBindingDescriptor& binding, InputValueType valueType, InputConsumption& consumption) const;

	ControlSample SampleControl(const InputControl& control, const InputConsumption& consumption) const;

	void ConsumeControl(const InputControl& control, InputConsumption& consumption) const;

	bool ModifiersHeld(EnumFlag<ModifierKeys> modifiers) const;

	PlayerInput* mPlayer;

	Guid mId;

	TString mName;

	int32_t mPriority;

	bool mEnabled = true;

	bool mActive = false;

	TArray<Scope<InputAction>> mActions;

	TArray<BindingEntry> mBindingOrder;

};

} // namespace Gleam
