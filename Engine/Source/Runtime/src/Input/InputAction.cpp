#include "gpch.h"
#include "InputAction.h"

#include <SDL3/SDL_keyboard.h>

using namespace Gleam;

static const InputInteraction kDefaultInteraction{};

static uint32_t ResolveLayoutKey(uint32_t code)
{
	if (code >= SDL_SCANCODE_A and code <= SDL_SCANCODE_Z)
	{
		const auto key = static_cast<SDL_Keycode>(SDLK_A + (code - SDL_SCANCODE_A));
		const auto scancode = SDL_GetScancodeFromKey(key, nullptr);
		if (scancode != SDL_SCANCODE_UNKNOWN)
		{
			return static_cast<uint32_t>(scancode);
		}
	}
	return code;
}

Float3 InputAction::ApplyProcessor(const InputProcessor& processor, const Float3& value, InputValueType valueType)
{
	switch (processor.type)
	{
		case InputProcessorType::DeadZone:
		{
			const float magnitude = ComputeMagnitude(value, valueType);
			if (magnitude < processor.min or magnitude <= 0.0f)
			{
				return Float3::zero;
			}
			const float range = Math::Max(processor.max - processor.min, 1e-5f);
			const float scaled = Math::Saturate((magnitude - processor.min) / range);
			return value * (scaled / magnitude);
		}
		case InputProcessorType::Scale:
		{
			return value * processor.factor;
		}
		case InputProcessorType::Invert:
		{
			return -value;
		}
		case InputProcessorType::Normalize:
		{
			const float magnitude = ComputeMagnitude(value, valueType);
			return magnitude > 0.0f ? value * (1.0f / magnitude) : value;
		}
		case InputProcessorType::Clamp:
		{
			return Float3(Math::Clamp(value.x, processor.min, processor.max),
						  Math::Clamp(value.y, processor.min, processor.max),
						  Math::Clamp(value.z, processor.min, processor.max));
		}
		default: return value;
	}
}

float InputAction::ComputeMagnitude(const Float3& value, InputValueType valueType)
{
	switch (valueType)
	{
		case InputValueType::Bool:
		case InputValueType::Float: return Math::Abs(value.x);
		case InputValueType::Float2: return Math::Sqrt(value.x * value.x + value.y * value.y);
		default: return Math::Length(value);
	}
}

InputAction::InputAction(InputActionMap* map, const InputActionDescriptor& descriptor)
	: mMap(map), mDescriptor(descriptor), mBindings(descriptor.bindings)
{
	ResolveBindings();
}

const Guid& InputAction::GetId() const
{
	return mDescriptor.id;
}

const TString& InputAction::GetName() const
{
	return mDescriptor.name;
}

InputValueType InputAction::GetValueType() const
{
	return mDescriptor.valueType;
}

InputActionMap* InputAction::GetMap() const
{
	return mMap;
}

const TArray<InputBindingDescriptor>& InputAction::GetBindings() const
{
	return mBindings;
}

InputActionPhase InputAction::GetPhase() const
{
	return mPhase;
}

bool InputAction::IsEnabled() const
{
	return mPhase != InputActionPhase::Disabled;
}

bool InputAction::IsPressed() const
{
	return mPressed;
}

bool InputAction::WasPressedThisFrame() const
{
	return mPressedThisFrame;
}

bool InputAction::WasReleasedThisFrame() const
{
	return mReleasedThisFrame;
}

bool InputAction::WasPerformedThisFrame() const
{
	return mPerformedThisFrame;
}

const InputDevice* InputAction::GetActiveDevice() const
{
	return mActiveDevice;
}

InputCallbackHandle InputAction::OnStarted(InputCallback&& callback)
{
	return AddCallback(InputActionPhase::Started, eastl::move(callback));
}

InputCallbackHandle InputAction::OnPerformed(InputCallback&& callback)
{
	return AddCallback(InputActionPhase::Performed, eastl::move(callback));
}

InputCallbackHandle InputAction::OnCanceled(InputCallback&& callback)
{
	return AddCallback(InputActionPhase::Canceled, eastl::move(callback));
}

void InputAction::RemoveCallback(InputCallbackHandle handle)
{
	auto it = eastl::find_if(mCallbacks.begin(), mCallbacks.end(), [handle](const Callback& callback)
	{
		return callback.handle == handle;
	});

	if (it != mCallbacks.end())
	{
		mCallbacks.erase(it);
	}
}

InputCallbackHandle InputAction::AddCallback(InputActionPhase phase, InputCallback&& callback)
{
	const auto handle = mNextCallbackHandle++;
	mCallbacks.push_back(Callback{ .handle = handle, .phase = phase, .function = eastl::move(callback) });
	return handle;
}

void InputAction::Enable()
{
	if (mPhase == InputActionPhase::Disabled)
	{
		mPhase = InputActionPhase::Waiting;
		mSyncHeldState = true;
	}
}

void InputAction::Disable(double time)
{
	if (mPhase == InputActionPhase::Started or mPhase == InputActionPhase::Performed)
	{
		Cancel(time);
	}

	mPhase = InputActionPhase::Disabled;
	mFrameState = InputBindingState();
	mValue = Float3::zero;
	mPerformedValue = Float3::zero;
	mMagnitude = 0.0f;
	mPressed = false;
	mPressedThisFrame = false;
	mReleasedThisFrame = false;
	mPerformedThisFrame = false;
	mTapCount = 0;
}

void InputAction::BeginFrame()
{
	mFrameState = InputBindingState();
}

void InputAction::Accumulate(const InputBindingState& state)
{
	const bool pressed = mFrameState.pressed or state.pressed;
	const bool released = mFrameState.released or state.released;
	if (state.magnitude > mFrameState.magnitude or (mFrameState.device == nullptr and state.device != nullptr))
	{
		mFrameState = state;
	}
	mFrameState.pressed = pressed;
	mFrameState.released = released;
}

void InputAction::Process(double time)
{
	mPressedThisFrame = false;
	mReleasedThisFrame = false;
	mPerformedThisFrame = false;

	if (mPhase == InputActionPhase::Canceled)
	{
		mPhase = InputActionPhase::Waiting;
	}

	auto value = mFrameState.value;
	for (const auto& processor : mDescriptor.processors)
	{
		value = ApplyProcessor(processor, value, mDescriptor.valueType);
	}
	mValue = value;
	mMagnitude = ComputeMagnitude(value, mDescriptor.valueType);

	if (mFrameState.device)
	{
		mActiveDevice = mFrameState.device;
	}
	if (mFrameState.magnitude > 0.0f or mFrameState.pressed)
	{
		mInteractionBinding = mFrameState.binding;
	}

	const auto& interaction = GetActiveInteraction();
	const bool pressed = mMagnitude >= interaction.pressPoint;
	if (mSyncHeldState)
	{
		mSyncHeldState = false;
		if (mFrameState.pressed == false)
		{
			mPressed = pressed;
		}
	}
	const bool tapped = mFrameState.pressed and pressed == false and mPressed == false;
	mPressedThisFrame = (pressed and mPressed == false) or tapped;
	mReleasedThisFrame = (pressed == false and mPressed) or tapped;
	mPressed = pressed;

	if (mDescriptor.valueType == InputValueType::Bool or HasInteractions())
	{
		ProcessButton(interaction, time);
	}
	else
	{
		ProcessValue(time);
	}
}

void InputAction::ProcessValue(double time)
{
	if (mMagnitude > 0.0f)
	{
		if (mPhase == InputActionPhase::Waiting)
		{
			Start(time);
			Perform(time);
		}
		else if (mValue != mPerformedValue)
		{
			Perform(time);
		}
	}
	else if (mPhase == InputActionPhase::Started or mPhase == InputActionPhase::Performed)
	{
		Cancel(time);
	}
}

void InputAction::ProcessButton(const InputInteraction& interaction, double time)
{
	if (mPressedThisFrame)
	{
		OnPress(interaction, time);
	}

	if (mPressed)
	{
		OnHeld(interaction, time);
	}
	else if (mReleasedThisFrame == false)
	{
		OnIdle(interaction, time);
	}

	if (mReleasedThisFrame)
	{
		OnRelease(interaction, time);
	}
}

void InputAction::OnPress(const InputInteraction& interaction, double time)
{
	switch (interaction.type)
	{
		case InputInteractionType::Press:
		{
			Start(time);
			Perform(time);
			break;
		}
		case InputInteractionType::MultiTap:
		{
			if (mPhase != InputActionPhase::Started)
			{
				mTapCount = 0;
				Start(time);
			}
			mInteractionStart = time;
			break;
		}
		default:
		{
			mInteractionStart = time;
			Start(time);
			break;
		}
	}
}

void InputAction::OnHeld(const InputInteraction& interaction, double time)
{
	if (mPhase != InputActionPhase::Started)
	{
		return;
	}

	const double elapsed = time - mInteractionStart;
	switch (interaction.type)
	{
		case InputInteractionType::Hold:
		{
			if (elapsed >= interaction.duration)
			{
				Perform(time);
			}
			break;
		}
		case InputInteractionType::Tap:
		case InputInteractionType::MultiTap:
		{
			if (elapsed > interaction.duration)
			{
				Cancel(time);
			}
			break;
		}
		default: break;
	}
}

void InputAction::OnIdle(const InputInteraction& interaction, double time)
{
	if (interaction.type == InputInteractionType::MultiTap and mPhase == InputActionPhase::Started and time - mLastRelease > interaction.duration)
	{
		Cancel(time);
	}
}

void InputAction::OnRelease(const InputInteraction& interaction, double time)
{
	switch (interaction.type)
	{
		case InputInteractionType::Release:
		{
			if (mPhase == InputActionPhase::Started)
			{
				Perform(time);
				mPhase = InputActionPhase::Waiting;
			}
			break;
		}
		case InputInteractionType::Tap:
		{
			if (mPhase == InputActionPhase::Started)
			{
				if (time - mInteractionStart <= interaction.duration)
				{
					Perform(time);
					mPhase = InputActionPhase::Waiting;
				}
				else
				{
					Cancel(time);
				}
			}
			break;
		}
		case InputInteractionType::MultiTap:
		{
			if (mPhase == InputActionPhase::Started)
			{
				if (time - mInteractionStart <= interaction.duration)
				{
					mLastRelease = time;
					if (++mTapCount >= Math::Max(interaction.tapCount, 1u))
					{
						Perform(time);
						mPhase = InputActionPhase::Waiting;
					}
				}
				else
				{
					Cancel(time);
				}
			}
			break;
		}
		default:
		{
			if (mPhase == InputActionPhase::Started or mPhase == InputActionPhase::Performed)
			{
				Cancel(time);
			}
			break;
		}
	}
}

void InputAction::Start(double time)
{
	mPhase = InputActionPhase::Started;
	Invoke(InputActionPhase::Started, time);
}

void InputAction::Perform(double time)
{
	mPhase = InputActionPhase::Performed;
	mPerformedThisFrame = true;
	mPerformedValue = mValue;
	Invoke(InputActionPhase::Performed, time);
}

void InputAction::Cancel(double time)
{
	mPhase = InputActionPhase::Canceled;
	mPerformedValue = Float3::zero;
	Invoke(InputActionPhase::Canceled, time);
}

void InputAction::Invoke(InputActionPhase phase, double time)
{
	TArray<InputCallback> callbacks;
	for (const auto& callback : mCallbacks)
	{
		if (callback.phase == phase)
		{
			callbacks.push_back(callback.function);
		}
	}

	const auto context = InputActionContext{ .action = this, .phase = phase, .device = mActiveDevice, .time = time };
	for (const auto& callback : callbacks)
	{
		callback(context);
	}
}

bool InputAction::HasInteractions() const
{
	if (mInteractionBinding < mResolvedBindings.size() and mResolvedBindings[mInteractionBinding].interactions.empty() == false)
	{
		return true;
	}
	return mDescriptor.interactions.empty() == false;
}

const InputInteraction& InputAction::GetActiveInteraction() const
{
	if (mInteractionBinding < mResolvedBindings.size() and mResolvedBindings[mInteractionBinding].interactions.empty() == false)
	{
		return mResolvedBindings[mInteractionBinding].interactions.front();
	}
	else if (mDescriptor.interactions.empty() == false)
	{
		return mDescriptor.interactions.front();
	}
	else
	{
		return kDefaultInteraction;
	}
}

void InputAction::ApplyOverride(uint32_t binding, uint32_t part, const InputControl& control)
{
	if (binding < mBindings.size())
	{
		auto& controls = mBindings[binding].controls;
		if (part >= controls.size())
		{
			controls.resize(part + 1);
		}
		controls[part] = control;
		ResolveBindings();
	}
}

void InputAction::ResetBindings()
{
	mBindings = mDescriptor.bindings;
	ResolveBindings();
}

void InputAction::ResolveBindings()
{
	mResolvedBindings = mBindings;
	for (auto& binding : mResolvedBindings)
	{
		if (binding.layoutMapped == false)
		{
			continue;
		}

		for (auto& control : binding.controls)
		{
			if (control.device == InputDeviceType::Keyboard)
			{
				control.code = ResolveLayoutKey(control.code);
			}
		}
	}
}
