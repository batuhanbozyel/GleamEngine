#pragma once
#include "InputActionAsset.h"

namespace Gleam {

class InputDevice;

enum class InputActionPhase
{
	Disabled,
	Waiting,
	Started,
	Performed,
	Canceled
};

struct InputEdges
{
	bool pressed = false;
	bool released = false;
	double pressTime = 0.0;
	double releaseTime = 0.0;

	void Merge(const InputEdges& other);
};

struct InputBindingState
{
	Float3 value = Float3::zero;
	float magnitude = 0.0f;
	InputEdges edges;
	const InputDevice* device = nullptr;
	uint32_t binding = 0;
};

struct InputActionState
{
	InputActionPhase phase = InputActionPhase::Disabled;
	InputBindingState frameState;
	uint32_t interactionBinding = 0;
	Float3 value = Float3::zero;
	Float3 performedValue = Float3::zero;
	float magnitude = 0.0f;
	const InputDevice* activeDevice = nullptr;
	double interactionStart = 0.0;
	double lastRelease = 0.0;
	uint32_t tapCount = 0;
	bool pressed = false;
	bool syncHeldState = false;
	bool pressedThisFrame = false;
	bool releasedThisFrame = false;
	bool performedThisFrame = false;

	template<typename T>
	T ReadValue() const;

	void Enable();

	void Disable();

	void Accumulate(const InputBindingState& state);

	void ProcessValue();

	void ProcessButton(const InputInteraction& interaction, double time, double pressTime, double releaseTime);

private:

	void OnPress(const InputInteraction& interaction, double time);

	void OnHeld(const InputInteraction& interaction, double time);

	void OnIdle(const InputInteraction& interaction, double time);

	void OnRelease(const InputInteraction& interaction, double time);

	void Start();

	void Perform();

	void Cancel();
};

template<>
inline bool InputActionState::ReadValue<bool>() const
{
	return pressed;
}

template<>
inline float InputActionState::ReadValue<float>() const
{
	return value.x;
}

template<>
inline Float2 InputActionState::ReadValue<Float2>() const
{
	return Float2(value.x, value.y);
}

template<>
inline Float3 InputActionState::ReadValue<Float3>() const
{
	return value;
}

struct InputActionMapState
{
	bool enabled = true;
	bool userActive = false;
	bool active = false;
};

struct InputActionStates
{
	const InputActionAsset* asset = nullptr;
	TArray<InputActionMapState> maps;
	TArray<InputActionState> actions;
	TArray<InputBindingDescriptor> bindings;
	uint32_t overridesVersion = 0;
	uint32_t keymapVersion = ~0u;
	uint32_t assetVersion = 0;

	InputActionStates() = default;

	explicit InputActionStates(const InputActionAsset* source);

	const InputActionState& GetAction(InputActionHandle action) const;

	void SetActionMapEnabled(InputActionMapHandle map, bool enabled);

	void SetUserActive(bool active);

	void ProcessAction(InputActionHandle action, double time);

private:

	void RefreshActionMap(InputActionMapHandle map);

	bool HasInteractions(InputActionHandle action) const;

	const InputInteraction& GetActiveInteraction(InputActionHandle action) const;
};

} // namespace Gleam
