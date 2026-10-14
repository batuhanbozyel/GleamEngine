#include "gpch.h"
#include "InputSystem.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Core/Events/InputEvent.h"
#include <SDL3/SDL.h>
#include <EASTL/sort.h>

using namespace Gleam;

static constexpr float kActuationThreshold = 0.5f;

void InputSystem::Initialize(Engine* engine)
{
	auto keyboard = CreateScope<Keyboard>(static_cast<uint32_t>(mDevices.size()));
	mKeyboard = keyboard.get();
	mDeviceViews.push_back(mKeyboard);
	mDevices.push_back(eastl::move(keyboard));

	auto mouse = CreateScope<Mouse>(static_cast<uint32_t>(mDevices.size()));
	mMouse = mouse.get();
	mDeviceViews.push_back(mMouse);
	mDevices.push_back(eastl::move(mouse));
}

void InputSystem::Shutdown(Engine* engine)
{
	if (SDL_WasInit(SDL_INIT_GAMEPAD))
	{
		for (auto gamepad : mGamepads)
		{
			gamepad->Disconnect();
		}
	}
	mPlayers.clear();
	mUsers.clear();
	mGamepads.clear();
	mDeviceViews.clear();
	mDevices.clear();
	mKeyboard = nullptr;
	mMouse = nullptr;
}

void InputSystem::Update()
{
	for (auto& device : mDevices)
	{
		device->Commit();
	}

	if (mKeymapChanged)
	{
		mKeymapChanged = false;
		for (auto& user : mUsers)
		{
			for (auto& map : user->mMaps)
			{
				map->ResolveBindings();
			}
		}
	}

	if (mJoinBehavior == PlayerJoinBehavior::JoinOnButtonPress)
	{
		ProcessJoins();
	}

	for (auto& user : mUsers)
	{
		user->UpdateRebind();
	}

	UpdateActionMaps(GetTime());
}

void InputSystem::UpdateActionMaps(double time)
{
	TArray<InputActionMap*> maps;
	for (auto& user : mUsers)
	{
		for (auto& map : user->mMaps)
		{
			if (map->IsActive())
			{
				maps.push_back(map.get());
			}
		}
	}

	eastl::stable_sort(maps.begin(), maps.end(), [](const InputActionMap* lhs, const InputActionMap* rhs)
	{
		return lhs->GetPriority() > rhs->GetPriority();
	});

	InputConsumption consumption;
	for (auto map : maps)
	{
		map->Update(consumption, time);
	}
}

void InputSystem::ProcessJoins()
{
	for (auto device : mDeviceViews)
	{
		if (device->IsConnected() == false or IsPairedToPlayer(device))
		{
			continue;
		}

		InputDevice* const candidates[] = { device };
		InputControl control;
		if (FindActuatedControl(TArrayView<InputDevice* const>(candidates, 1), control) == false)
		{
			continue;
		}

		auto player = CreatePlayer();
		if (device->GetType() == InputDeviceType::Gamepad)
		{
			player->PairDevice(device);
		}
		else
		{
			player->PairDevice(mKeyboard);
			player->PairDevice(mMouse);
		}
	}
}

double InputSystem::GetTime()
{
	return static_cast<double>(SDL_GetTicksNS()) * 1e-9;
}

bool InputSystem::FindActuatedControl(TArrayView<InputDevice* const> devices, InputControl& control)
{
	for (const auto device : devices)
	{
		if (device->IsConnected() == false)
		{
			continue;
		}

		for (const auto code : InputControl::GetCodes(device->GetType()))
		{
			const auto candidate = InputControl{ .device = device->GetType(), .code = code };
			if (candidate.IsButton())
			{
				if (device->WasControlPressedThisFrame(code))
				{
					control = candidate;
					return true;
				}
			}
			else if (candidate.device == InputDeviceType::Gamepad)
			{
				const auto stick = static_cast<GamepadControl>(code);
				if ((stick == GamepadControl::LeftStick or stick == GamepadControl::RightStick) and Math::Length(device->ReadControl(code)) >= kActuationThreshold)
				{
					control = candidate;
					return true;
				}
			}
		}
	}
	return false;
}

TArrayView<InputDevice* const> InputSystem::GetDevices() const
{
	return TArrayView<InputDevice* const>(mDeviceViews.data(), mDeviceViews.size());
}

PlayerInput* InputSystem::CreatePlayer()
{
	auto player = mUsers.emplace_back(CreateScope<PlayerInput>(FindFreePlayerIndex())).get();
	mPlayers.push_back(player);
	eastl::stable_sort(mPlayers.begin(), mPlayers.end(), [](const PlayerInput* lhs, const PlayerInput* rhs)
	{
		return lhs->GetPlayerIndex() < rhs->GetPlayerIndex();
	});

	if (mJoinBehavior == PlayerJoinBehavior::Manual and mPlayers.size() == 1)
	{
		for (auto device : mDeviceViews)
		{
			player->PairDevice(device);
		}
		player->mPairNewDevices = true;
	}

	EventDispatcher<PlayerJoinedEvent>::Publish(PlayerJoinedEvent(player));
	return player;
}

PlayerInput* InputSystem::CreateUser()
{
	return mUsers.emplace_back(CreateScope<PlayerInput>(PlayerInput::InvalidPlayerIndex)).get();
}

void InputSystem::DestroyPlayer(PlayerInput* player)
{
	auto it = eastl::find_if(mUsers.begin(), mUsers.end(), [player](const Scope<PlayerInput>& user)
	{
		return user.get() == player;
	});

	if (it == mUsers.end())
	{
		return;
	}

	if (player->GetPlayerIndex() != PlayerInput::InvalidPlayerIndex)
	{
		EventDispatcher<PlayerLeftEvent>::Publish(PlayerLeftEvent(player));
		mPlayers.erase(eastl::find(mPlayers.begin(), mPlayers.end(), player));
	}

	player->CancelInteractiveRebind();
	player->SetEnabled(false);
	mUsers.erase(it);
}

void InputSystem::DestroyPlayers()
{
	while (mPlayers.empty() == false)
	{
		DestroyPlayer(mPlayers.back());
	}
}

PlayerInput* InputSystem::GetPlayer(uint32_t index) const
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

const TArray<PlayerInput*>& InputSystem::GetPlayers() const
{
	return mPlayers;
}

PlayerJoinBehavior InputSystem::GetJoinBehavior() const
{
	return mJoinBehavior;
}

void InputSystem::SetJoinBehavior(PlayerJoinBehavior behavior)
{
	mJoinBehavior = behavior;
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

void InputSystem::ShowCursor() const
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
	SDL_WarpMouseInWindow(windowSystem->GetSDLWindow(), mCursorHidePosition.x, mCursorHidePosition.y);
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), false);
    SDL_ShowCursor();
	mCursorHidden = false;
}

void InputSystem::HideCursor() const
{
	static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    SDL_HideCursor();
	SDL_SetWindowRelativeMouseMode(windowSystem->GetSDLWindow(), true);
	SDL_GetMouseState(&mCursorHidePosition.x, &mCursorHidePosition.y);
	mCursorHidden = true;
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
			mKeymapChanged = true;
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
	for (const auto& device : mDevices)
	{
		if (device->GetType() == InputDeviceType::Gamepad and device->IsConnected() == false)
		{
			auto candidate = static_cast<Gamepad*>(device.get());
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
		auto device = CreateScope<Gamepad>(static_cast<uint32_t>(mDevices.size()), handle);
		gamepad = device.get();
		mDeviceViews.push_back(gamepad);
		mDevices.push_back(eastl::move(device));
	}

	mGamepads.push_back(gamepad);
	for (auto& user : mUsers)
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
