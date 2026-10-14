#pragma once
#include "InputActionDescriptor.h"

#include <functional>

namespace Gleam {

class InputAction;
class InputActionMap;

enum class InputActionPhase
{
	Disabled,
	Waiting,
	Started,
	Performed,
	Canceled
};

struct InputActionContext
{
	InputAction* action = nullptr;
	InputActionPhase phase = InputActionPhase::Waiting;
	const InputDevice* device = nullptr;
	double time = 0.0;
};

using InputCallback = std::function<void(const InputActionContext&)>;

using InputCallbackHandle = uint32_t;

struct InputBindingState
{
	Float3 value = Float3::zero;
	float magnitude = 0.0f;
	bool pressed = false;
	bool released = false;
	const InputDevice* device = nullptr;
	uint32_t binding = 0;
};

class InputAction
{
	friend class InputActionMap;
	friend class PlayerInput;
public:

	InputAction(InputActionMap* map, const InputActionDescriptor& descriptor);

	const Guid& GetId() const;

	const TString& GetName() const;

	InputValueType GetValueType() const;

	InputActionMap* GetMap() const;

	const TArray<InputBindingDescriptor>& GetBindings() const;

	InputActionPhase GetPhase() const;

	bool IsEnabled() const;

	bool IsPressed() const;

	bool WasPressedThisFrame() const;

	bool WasReleasedThisFrame() const;

	bool WasPerformedThisFrame() const;

	template<typename T>
	T ReadValue() const;

	const InputDevice* GetActiveDevice() const;

	InputCallbackHandle OnStarted(InputCallback&& callback);

	InputCallbackHandle OnPerformed(InputCallback&& callback);

	InputCallbackHandle OnCanceled(InputCallback&& callback);

	void RemoveCallback(InputCallbackHandle handle);

private:

	struct Callback
	{
		InputCallbackHandle handle;
		InputActionPhase phase;
		InputCallback function;
	};

	static Float3 ApplyProcessor(const InputProcessor& processor, const Float3& value, InputValueType valueType);

	static float ComputeMagnitude(const Float3& value, InputValueType valueType);

	InputCallbackHandle AddCallback(InputActionPhase phase, InputCallback&& callback);

	void Enable();

	void Disable(double time);

	void BeginFrame();

	void Accumulate(const InputBindingState& state);

	void Process(double time);

	void ProcessValue(double time);

	void ProcessButton(const InputInteraction& interaction, double time);

	void OnPress(const InputInteraction& interaction, double time);

	void OnHeld(const InputInteraction& interaction, double time);

	void OnIdle(const InputInteraction& interaction, double time);

	void OnRelease(const InputInteraction& interaction, double time);

	void Start(double time);

	void Perform(double time);

	void Cancel(double time);

	void Invoke(InputActionPhase phase, double time);

	bool HasInteractions() const;

	const InputInteraction& GetActiveInteraction() const;

	void ApplyOverride(uint32_t binding, uint32_t part, const InputControl& control);

	void ResetBindings();

	void ResolveBindings();

	InputActionMap* mMap;

	InputActionDescriptor mDescriptor;

	TArray<InputBindingDescriptor> mBindings;

	TArray<InputBindingDescriptor> mResolvedBindings;

	TArray<Callback> mCallbacks;

	InputCallbackHandle mNextCallbackHandle = 0;

	InputActionPhase mPhase = InputActionPhase::Disabled;

	InputBindingState mFrameState;

	uint32_t mInteractionBinding = 0;

	Float3 mValue = Float3::zero;

	Float3 mPerformedValue = Float3::zero;

	float mMagnitude = 0.0f;

	const InputDevice* mActiveDevice = nullptr;

	bool mPressed = false;

	bool mSyncHeldState = false;

	bool mPressedThisFrame = false;

	bool mReleasedThisFrame = false;

	bool mPerformedThisFrame = false;

	double mInteractionStart = 0.0;

	double mLastRelease = 0.0;

	uint32_t mTapCount = 0;

};

template<>
inline bool InputAction::ReadValue<bool>() const
{
	return IsPressed();
}

template<>
inline float InputAction::ReadValue<float>() const
{
	return mValue.x;
}

template<>
inline Float2 InputAction::ReadValue<Float2>() const
{
	return Float2(mValue.x, mValue.y);
}

template<>
inline Float3 InputAction::ReadValue<Float3>() const
{
	return mValue;
}

} // namespace Gleam
