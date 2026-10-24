#include "gpch.h"
#include "InputActionState.h"

using namespace Gleam;

static const InputInteraction kDefaultInteraction{};

InputActionStates::InputActionStates(const InputActionAsset* source)
	: asset(source)
	, maps(source->GetActionMapCount())
	, actions(source->GetActionCount())
	, bindings(source->GetBindings())
	, assetVersion(source->GetVersion())
{

}

const InputActionState& InputActionStates::GetAction(InputActionHandle action) const
{
	return actions[action.value];
}

void InputActionStates::SetActionMapEnabled(InputActionMapHandle map, bool enabled)
{
	maps[map.value].enabled = enabled;
	RefreshActionMap(map);
}

void InputActionStates::SetUserActive(bool active)
{
	for (uint32_t i = 0; i < maps.size(); ++i)
	{
		maps[i].userActive = active;
		RefreshActionMap(InputActionMapHandle{ i });
	}
}

void InputActionStates::ProcessAction(InputActionHandle handle, double time)
{
	const auto& descriptor = asset->GetAction(handle);
	auto& action = actions[handle.value];
	action.pressedThisFrame = false;
	action.releasedThisFrame = false;
	action.performedThisFrame = false;

	if (action.phase == InputActionPhase::Canceled)
	{
		action.phase = InputActionPhase::Waiting;
	}

	auto value = action.frameState.value;
	for (const auto& processor : descriptor.processors)
	{
		value = processor.Apply(value, descriptor);
	}
	action.value = value;
	action.magnitude = descriptor.ComputeMagnitude(value);

	if (action.frameState.device)
	{
		action.activeDevice = action.frameState.device;
	}
	if (action.frameState.magnitude > 0.0f or action.frameState.pressed)
	{
		action.interactionBinding = action.frameState.binding;
	}

	const auto& interaction = GetActiveInteraction(handle);
	const bool pressed = action.magnitude >= interaction.pressPoint;
	if (action.syncHeldState)
	{
		action.syncHeldState = false;
		if (action.frameState.pressed == false)
		{
			action.pressed = pressed;
		}
	}
	const bool tapped = action.frameState.pressed and pressed == false and action.pressed == false;
	action.pressedThisFrame = (pressed and action.pressed == false) or tapped;
	action.releasedThisFrame = (pressed == false and action.pressed) or tapped;
	action.pressed = pressed;

	if (descriptor.valueType == InputValueType::Bool or HasInteractions(handle))
	{
		action.ProcessButton(interaction, time);
	}
	else
	{
		action.ProcessValue();
	}
}

void InputActionStates::RefreshActionMap(InputActionMapHandle map)
{
	auto& mapState = maps[map.value];
	const bool active = mapState.enabled and mapState.userActive;
	if (active != mapState.active)
	{
		mapState.active = active;
		const auto& layout = asset->GetActionMapLayout(map);
		for (uint32_t i = layout.firstAction; i < layout.firstAction + layout.actionCount; ++i)
		{
			if (active)
			{
				actions[i].Enable();
			}
			else
			{
				actions[i].Disable();
			}
		}
	}
}

bool InputActionStates::HasInteractions(InputActionHandle action) const
{
	const auto& layout = asset->GetActionLayout(action);
	const uint32_t binding = actions[action.value].interactionBinding;
	if (binding < layout.bindingCount and bindings[layout.firstBinding + binding].interactions.empty() == false)
	{
		return true;
	}
	return asset->GetAction(action).interactions.empty() == false;
}

const InputInteraction& InputActionStates::GetActiveInteraction(InputActionHandle action) const
{
	const auto& layout = asset->GetActionLayout(action);
	const auto& descriptor = asset->GetAction(action);
	const uint32_t binding = actions[action.value].interactionBinding;
	if (binding < layout.bindingCount and bindings[layout.firstBinding + binding].interactions.empty() == false)
	{
		return bindings[layout.firstBinding + binding].interactions.front();
	}
	else if (descriptor.interactions.empty() == false)
	{
		return descriptor.interactions.front();
	}
	else
	{
		return kDefaultInteraction;
	}
}

void InputActionState::Enable()
{
	if (phase == InputActionPhase::Disabled)
	{
		phase = InputActionPhase::Waiting;
		syncHeldState = true;
	}
}

void InputActionState::Disable()
{
	phase = InputActionPhase::Disabled;
	frameState = InputBindingState();
	value = Float3::zero;
	performedValue = Float3::zero;
	magnitude = 0.0f;
	pressed = false;
	pressedThisFrame = false;
	releasedThisFrame = false;
	performedThisFrame = false;
	tapCount = 0;
}

void InputActionState::Accumulate(const InputBindingState& state)
{
	const bool anyPressed = frameState.pressed or state.pressed;
	const bool anyReleased = frameState.released or state.released;
	if (state.magnitude > frameState.magnitude or (frameState.device == nullptr and state.device != nullptr))
	{
		frameState = state;
	}
	frameState.pressed = anyPressed;
	frameState.released = anyReleased;
}

void InputActionState::ProcessValue()
{
	if (magnitude > 0.0f)
	{
		if (phase == InputActionPhase::Waiting)
		{
			Start();
			Perform();
		}
		else if (value != performedValue)
		{
			Perform();
		}
	}
	else if (phase == InputActionPhase::Started or phase == InputActionPhase::Performed)
	{
		Cancel();
	}
}

void InputActionState::ProcessButton(const InputInteraction& interaction, double time)
{
	if (pressedThisFrame)
	{
		OnPress(interaction, time);
	}

	if (pressed)
	{
		OnHeld(interaction, time);
	}
	else if (releasedThisFrame == false)
	{
		OnIdle(interaction, time);
	}

	if (releasedThisFrame)
	{
		OnRelease(interaction, time);
	}
}

void InputActionState::OnPress(const InputInteraction& interaction, double time)
{
	switch (interaction.type)
	{
		case InputInteractionType::Press:
		{
			Start();
			Perform();
			break;
		}
		case InputInteractionType::MultiTap:
		{
			if (phase != InputActionPhase::Started)
			{
				tapCount = 0;
				Start();
			}
			interactionStart = time;
			break;
		}
		default:
		{
			interactionStart = time;
			Start();
			break;
		}
	}
}

void InputActionState::OnHeld(const InputInteraction& interaction, double time)
{
	if (phase == InputActionPhase::Started)
	{
		const double elapsed = time - interactionStart;
		switch (interaction.type)
		{
			case InputInteractionType::Hold:
			{
				if (elapsed >= interaction.duration)
				{
					Perform();
				}
				break;
			}
			case InputInteractionType::Tap:
			case InputInteractionType::MultiTap:
			{
				if (elapsed > interaction.duration)
				{
					Cancel();
				}
				break;
			}
			default: break;
		}
	}
}

void InputActionState::OnIdle(const InputInteraction& interaction, double time)
{
	if (interaction.type == InputInteractionType::MultiTap and phase == InputActionPhase::Started and time - lastRelease > interaction.duration)
	{
		Cancel();
	}
}

void InputActionState::OnRelease(const InputInteraction& interaction, double time)
{
	switch (interaction.type)
	{
		case InputInteractionType::Release:
		{
			if (phase == InputActionPhase::Started)
			{
				Perform();
				phase = InputActionPhase::Waiting;
			}
			break;
		}
		case InputInteractionType::Tap:
		{
			if (phase == InputActionPhase::Started)
			{
				if (time - interactionStart <= interaction.duration)
				{
					Perform();
					phase = InputActionPhase::Waiting;
				}
				else
				{
					Cancel();
				}
			}
			break;
		}
		case InputInteractionType::MultiTap:
		{
			if (phase == InputActionPhase::Started)
			{
				if (time - interactionStart <= interaction.duration)
				{
					lastRelease = time;
					if (++tapCount >= Math::Max(interaction.tapCount, 1u))
					{
						Perform();
						phase = InputActionPhase::Waiting;
					}
				}
				else
				{
					Cancel();
				}
			}
			break;
		}
		default:
		{
			if (phase == InputActionPhase::Started or phase == InputActionPhase::Performed)
			{
				Cancel();
			}
			break;
		}
	}
}

void InputActionState::Start()
{
	phase = InputActionPhase::Started;
}

void InputActionState::Perform()
{
	phase = InputActionPhase::Performed;
	performedThisFrame = true;
	performedValue = value;
}

void InputActionState::Cancel()
{
	phase = InputActionPhase::Canceled;
	performedValue = Float3::zero;
}
