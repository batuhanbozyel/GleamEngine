#include "gpch.h"
#include "InputSystem.h"
#include "InputUtils.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Core/Events/InputEvent.h"
#include <SDL3/SDL.h>
#include <EASTL/sort.h>
#include <EASTL/algorithm.h>

using namespace Gleam;

static double ToSeconds(uint64_t nanoseconds)
{
	return static_cast<double>(nanoseconds) * 1e-9;
}

static uint64_t ConsumptionKey(const InputDevice* device, uint32_t code)
{
	return (static_cast<uint64_t>(device->GetId()) << 32) | code;
}

static uint64_t ControlKey(const InputControl& control)
{
	return (static_cast<uint64_t>(control.device) << 32) | control.code;
}

void InputSystem::Initialize(Engine* engine)
{
	mKeyboard = new Keyboard(static_cast<uint32_t>(mDevices.size()));
	mDevices.push_back(mKeyboard);

	mMouse = new Mouse(static_cast<uint32_t>(mDevices.size()));
	mDevices.push_back(mMouse);
}

void InputSystem::Shutdown(Engine* engine)
{
	if (SDL_WasInit(SDL_INIT_GAMEPAD))
	{
		for (auto gamepad : mGamepads)
		{
			gamepad->Disconnect();
		}
		mGamepads.clear();
	}
	mPendingDestroys.clear();
	mPlayers.clear();

	for (auto user : mUsers)
	{
		delete user;
	}
	mUsers.clear();

	for (auto device : mDevices)
	{
		delete device;
	}
	mDevices.clear();

	mKeyboard = nullptr;
	mMouse = nullptr;
}

void InputSystem::Update()
{
	mUpdating = true;
	for (auto user : mUsers)
	{
		user->UpdateRebind();
	}
	mUpdating = false;

	for (auto user : mPendingDestroys)
	{
		ReleaseUser(user);
	}
	mPendingDestroys.clear();

	for (auto device : mDevices)
	{
		device->Commit();
	}
	mConsumedControls.clear();
}

void InputSystem::UpdateActions(InputActionStates& states, const InputUser* user)
{
	if (PrepareActions(states, user))
	{
		const double time = GetTime();
		for (const auto map : states.asset->GetActionMapOrder())
		{
			if (states.maps[map.value].active)
			{
				UpdateActionMap(states, map, *user, time);
			}
		}
	}
}

bool InputSystem::PrepareActions(InputActionStates& states, const InputUser* user)
{
	const bool userActive = user and user->IsEnabled() and user->IsRebinding() == false;
	states.SetUserActive(userActive);

	if (userActive and (states.overridesVersion != user->GetBindingOverridesVersion() or states.keymapVersion != mKeymapVersion))
	{
		ResolveBindings(states, *user);
	}
	return userActive;
}

InputUser* InputSystem::JoinPlayer(uint32_t index)
{
	const bool firstPlayer = mPlayers.empty();
	auto player = CreatePlayer(index);
	if (firstPlayer)
	{
		for (auto device : mDevices)
		{
			player->PairDevice(device);
		}
		player->mPairNewDevices = true;
	}
	else
	{
		GLEAM_CORE_WARN("Player {0} has no paired devices. Use JoinOnButtonPress on the PlayerInputManager to join additional players.", index);
	}
	return player;
}

void InputSystem::ProcessJoins(uint32_t maxPlayers)
{
	for (auto device : mDevices)
	{
		if (mPlayers.size() >= maxPlayers or device->IsConnected() == false or IsPairedToPlayer(device))
		{
			continue;
		}

		InputDevice* const candidates[] = { device };
		InputControl control;
		if (InputUtils::FindActuatedControl(TArrayView<InputDevice* const>(candidates, 1), control) == false)
		{
			continue;
		}

		auto player = CreatePlayer(FindFreePlayerIndex());
		if (device->GetType() == InputDeviceType::Gamepad)
		{
			player->PairDevice(device);
		}
		else
		{
			player->PairDevice(mKeyboard);
			player->PairDevice(mMouse);
		}
		EventDispatcher<PlayerJoinedEvent>::Publish(PlayerJoinedEvent(player));
	}
}

void InputSystem::ResolveBindings(InputActionStates& states, const InputUser& user) const
{
	const auto& asset = *states.asset;
	states.bindings = asset.GetBindings();
	for (const auto& entry : user.GetBindingOverrides().overrides)
	{
		const auto action = asset.FindAction(entry.map, entry.action);
		if (action.IsValid() and entry.binding < asset.GetActionLayout(action).bindingCount)
		{
			auto& controls = states.bindings[asset.GetActionLayout(action).firstBinding + entry.binding].controls;
			if (entry.part >= controls.size())
			{
				controls.resize(entry.part + 1);
			}
			controls[entry.part] = entry.control;
		}
	}

	for (auto& binding : states.bindings)
	{
		if (binding.layoutMapped)
		{
			for (auto& control : binding.controls)
			{
				if (control.device == InputDeviceType::Keyboard)
				{
					control.code = InputUtils::ResolveLayoutKey(control.code);
				}
			}
		}
	}

	states.overridesVersion = user.GetBindingOverridesVersion();
	states.keymapVersion = mKeymapVersion;
}

void InputSystem::UpdateActionMap(InputActionStates& states, InputActionMapHandle map, const InputUser& user, double time)
{
	const auto& asset = *states.asset;
	const auto& layout = asset.GetActionMapLayout(map);
	const auto& mapId = asset.GetActionMap(map).id;
	const auto& devices = user.GetPairedDevices();

	for (uint32_t i = layout.firstAction; i < layout.firstAction + layout.actionCount; ++i)
	{
		states.actions[i].frameState = InputBindingState();
	}

	// Copies of a map don't consume from each other, so controls consumed earlier in this map are hidden from the
	// rest of it here instead: Ctrl+Shift+Z must not also trigger Ctrl+Z.
	mMapConsumedControls.clear();
	for (const auto& entry : layout.bindingOrder)
	{
		auto state = EvaluateBinding(states.bindings[entry.binding], asset.GetAction(entry.action), mapId, devices);
		state.binding = entry.binding - asset.GetActionLayout(entry.action).firstBinding;
		states.actions[entry.action.value].Accumulate(state);
	}

	for (uint32_t i = layout.firstAction; i < layout.firstAction + layout.actionCount; ++i)
	{
		states.ProcessAction(InputActionHandle{ i }, time);
	}
}

InputBindingState InputSystem::EvaluateBinding(const InputBindingDescriptor& binding, const InputActionDescriptor& action, const Guid& map, const TArray<InputDevice*>& devices)
{
	InputBindingState state;
	const uint32_t partCount = InputBindingDescriptor::GetPartCount(binding.type);
	if (partCount == 0 or binding.controls.size() < partCount or InputUtils::ModifiersHeld(binding.modifiers, TArrayView<InputDevice* const>(devices.data(), devices.size())) == false)
	{
		return state;
	}

	TFixedArray<ControlSample, 6> samples;
	for (uint32_t i = 0; i < partCount; ++i)
	{
		samples.push_back(SampleControl(binding.controls[i], map, devices));
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
		state.edges.Merge(sample.edges);
		if (sample.device and (state.device == nullptr or sample.magnitude > strongest))
		{
			state.device = sample.device;
			strongest = sample.magnitude;
		}
	}

	for (const auto& processor : binding.processors)
	{
		state.value = processor.Apply(state.value, action);
	}
	state.magnitude = action.ComputeMagnitude(state.value);

	if (state.magnitude > 0.0f or state.edges.pressed)
	{
		for (uint32_t i = 0; i < partCount; ++i)
		{
			if (samples[i].magnitude > 0.0f or samples[i].edges.pressed)
			{
				ConsumeControl(binding.controls[i], map, devices);
			}
		}
	}
	return state;
}

InputSystem::ControlSample InputSystem::SampleControl(const InputControl& control, const Guid& map, const TArray<InputDevice*>& devices) const
{
	ControlSample sample;
	if (mMapConsumedControls.find(ControlKey(control)) != mMapConsumedControls.end())
	{
		return sample;
	}

	for (const auto device : devices)
	{
		if (device->GetType() != control.device or device->IsConnected() == false)
		{
			continue;
		}

		const auto consumed = mConsumedControls.find(ConsumptionKey(device, control.code));
		if (consumed != mConsumedControls.end() and consumed->second != map)
		{
			continue;
		}

		const auto value = device->ReadControl(control.code);
		const float magnitude = Math::Length(value);
		const InputEdges edges{
			.pressed = device->WasControlPressedThisFrame(control.code),
			.released = device->WasControlReleasedThisFrame(control.code),
			.pressTime = ToSeconds(device->GetControlPressTime(control.code)),
			.releaseTime = ToSeconds(device->GetControlReleaseTime(control.code))
		};
		if (magnitude > sample.magnitude or (sample.device == nullptr and (edges.pressed or edges.released)))
		{
			sample.value = value;
			sample.magnitude = magnitude;
			sample.device = device;
		}
		sample.edges.Merge(edges);
	}
	return sample;
}

void InputSystem::ConsumeControl(const InputControl& control, const Guid& map, const TArray<InputDevice*>& devices)
{
	mMapConsumedControls.insert(ControlKey(control));
	for (const auto device : devices)
	{
		if (device->GetType() == control.device)
		{
			mConsumedControls.emplace(ConsumptionKey(device, control.code), map);
		}
	}
}

double InputSystem::GetTime()
{
	return ToSeconds(SDL_GetTicksNS());
}

TArrayView<InputDevice* const> InputSystem::GetDevices() const
{
	return TArrayView<InputDevice* const>(mDevices.data(), mDevices.size());
}

InputUser* InputSystem::CreatePlayer(uint32_t index)
{
	auto player = new InputUser(index);
	mUsers.push_back(player);
	mPlayers.push_back(player);
	eastl::stable_sort(mPlayers.begin(), mPlayers.end(), [](const InputUser* lhs, const InputUser* rhs)
	{
		return lhs->GetPlayerIndex() < rhs->GetPlayerIndex();
	});
	return player;
}

InputUser* InputSystem::CreateUser()
{
	auto user = new InputUser(InputUser::InvalidPlayerIndex);
	mUsers.push_back(user);
	return user;
}

void InputSystem::DestroyUser(InputUser* user)
{
	if (mUpdating)
	{
		mPendingDestroys.push_back(user);
	}
	else
	{
		ReleaseUser(user);
	}
}

void InputSystem::DestroyPlayers()
{
	const auto players = mPlayers;
	for (auto player : players)
	{
		DestroyUser(player);
	}
}

void InputSystem::ReleaseUser(InputUser* user)
{
	auto it = eastl::find(mUsers.begin(), mUsers.end(), user);
	if (it == mUsers.end())
	{
		return;
	}

	if (user->GetPlayerIndex() != InputUser::InvalidPlayerIndex)
	{
		EventDispatcher<PlayerLeftEvent>::Publish(PlayerLeftEvent(user));
		mPlayers.erase(eastl::find(mPlayers.begin(), mPlayers.end(), user));
	}

	user->CancelInteractiveRebind();
	mUsers.erase(it);
	delete user;
}

InputUser* InputSystem::GetPlayer(uint32_t index) const
{
	for (auto player : mPlayers)
	{
		if (player->GetPlayerIndex() == index)
		{
			return player;
		}
	}
	return nullptr;
}

const TArray<InputUser*>& InputSystem::GetPlayers() const
{
	return mPlayers;
}

bool InputSystem::IsPairedToPlayer(const InputDevice* device) const
{
	for (auto player : mPlayers)
	{
		if (player->IsPaired(device))
		{
			return true;
		}
	}
	return false;
}

uint32_t InputSystem::FindFreePlayerIndex() const
{
	uint32_t index = 0;
	while (GetPlayer(index))
	{
		++index;
	}
	return index;
}

Keyboard* InputSystem::GetKeyboard() const
{
	return mKeyboard;
}

Mouse* InputSystem::GetMouse() const
{
	return mMouse;
}

const TArray<Gamepad*>& InputSystem::GetGamepads() const
{
	return mGamepads;
}

void InputSystem::ShowCursor()
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
	SDL_WarpMouseInWindow(windowSystem->GetSDLWindow(), mCursorHidePosition.x, mCursorHidePosition.y);
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), false);
	SDL_ShowCursor();
}

void InputSystem::HideCursor()
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
	SDL_GetMouseState(&mCursorHidePosition.x, &mCursorHidePosition.y);
	SDL_HideCursor();
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), true);
}

bool InputSystem::CursorVisible() const
{
	return SDL_CursorVisible();
}

bool InputSystem::ProcessEvent(const SDL_Event& e)
{
	switch (e.type)
	{
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		{
			mKeyboard->ProcessEvent(e.key);
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
		{
			mMouse->ProcessMotionEvent(e.motion);
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
			mMouse->ProcessButtonEvent(e.button);
			break;
		}
		case SDL_EVENT_MOUSE_WHEEL:
		{
			mMouse->ProcessWheelEvent(e.wheel);
			break;
		}
		case SDL_EVENT_KEYMAP_CHANGED:
		{
			++mKeymapVersion;
			break;
		}
		case SDL_EVENT_GAMEPAD_ADDED:
		{
			ConnectGamepad(e.gdevice.which);
			break;
		}
		case SDL_EVENT_GAMEPAD_REMOVED:
		{
			DisconnectGamepad(e.gdevice.which);
			break;
		}
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
		{
			if (auto gamepad = FindGamepad(e.gbutton.which))
			{
				gamepad->ProcessButtonEvent(e.gbutton);
			}
			break;
		}
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
		{
			if (auto gamepad = FindGamepad(e.gaxis.which))
			{
				gamepad->ProcessAxisEvent(e.gaxis);
			}
			break;
		}
		default: break;
	}
	return e.type >= SDL_EVENT_KEY_DOWN and e.type < SDL_EVENT_FINGER_DOWN;
}

void InputSystem::ConnectGamepad(SDL_JoystickID joystickId)
{
	if (FindGamepad(joystickId))
	{
		return;
	}

	auto handle = SDL_OpenGamepad(joystickId);
	if (handle == nullptr)
	{
		GLEAM_CORE_ERROR("Gamepad could not be opened: {0}", SDL_GetError());
		return;
	}

	Gamepad* gamepad = nullptr;
	const auto guid = SDL_GetGamepadGUIDForID(joystickId);
	for (auto device : mDevices)
	{
		if (device->GetType() == InputDeviceType::Gamepad and device->IsConnected() == false)
		{
			auto candidate = static_cast<Gamepad*>(device);
			if (candidate->Matches(guid))
			{
				gamepad = candidate;
				break;
			}
		}
	}

	if (gamepad)
	{
		gamepad->Connect(handle);
	}
	else
	{
		gamepad = new Gamepad(static_cast<uint32_t>(mDevices.size()), handle);
		mDevices.push_back(gamepad);
	}

	mGamepads.push_back(gamepad);
	for (auto user : mUsers)
	{
		if (user->mPairNewDevices)
		{
			user->PairDevice(gamepad);
		}
	}
	EventDispatcher<InputDeviceConnectedEvent>::Publish(InputDeviceConnectedEvent(gamepad));
}

void InputSystem::DisconnectGamepad(SDL_JoystickID joystickId)
{
	auto gamepad = FindGamepad(joystickId);
	if (gamepad == nullptr)
	{
		return;
	}

	gamepad->Disconnect();
	mGamepads.erase(eastl::find(mGamepads.begin(), mGamepads.end(), gamepad));
	EventDispatcher<InputDeviceDisconnectedEvent>::Publish(InputDeviceDisconnectedEvent(gamepad));
}

Gamepad* InputSystem::FindGamepad(SDL_JoystickID joystickId) const
{
	for (auto gamepad : mGamepads)
	{
		if (gamepad->GetJoystickId() == joystickId)
		{
			return gamepad;
		}
	}
	return nullptr;
}
