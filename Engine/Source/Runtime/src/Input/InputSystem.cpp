#include "gpch.h"
#include "InputSystem.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/WindowSystem.h"
#include "Core/Events/InputEvent.h"
#include "Assets/AssetManager.h"
#include "World/World.h"
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
	mPlayerInputs.clear();
	mPendingDestroys.clear();
	mPlayers.clear();
	mUsers.clear();
	mGamepads.clear();
	mDeviceViews.clear();
	mDevices.clear();
	mKeyboard = nullptr;
	mMouse = nullptr;
}

void InputSystem::Update(World* world)
{
	for (auto& device : mDevices)
	{
		device->Commit();
	}

	TArray<PlayerInput*> inputs = mPlayerInputs;
	if (world and mWorldInputEnabled)
	{
		auto& entityManager = world->GetEntityManager();

		const PlayerInputManager* manager = nullptr;
		entityManager.ForEach<PlayerInputManager>([&manager](const PlayerInputManager& component)
		{
			if (manager == nullptr)
			{
				manager = &component;
			}
		});

		const auto joinBehavior = manager ? manager->joinBehavior : PlayerJoinBehavior::Manual;
		if (joinBehavior == PlayerJoinBehavior::JoinOnButtonPress)
		{
			ProcessJoins(manager->maxPlayers);
		}

		TArray<InputUser*> joined;
		entityManager.ForEach<PlayerInput>([&](PlayerInput& input)
		{
			if (auto player = SyncPlayerInput(input, joinBehavior))
			{
				joined.push_back(player);
			}
		});

		for (auto player : joined)
		{
			EventDispatcher<PlayerJoinedEvent>::Publish(PlayerJoinedEvent(player));
		}

		entityManager.ForEach<PlayerInput>([&inputs](PlayerInput& input)
		{
			inputs.push_back(&input);
		});
	}

	if (mKeymapChanged)
	{
		mKeymapChanged = false;
		for (auto input : inputs)
		{
			for (auto& map : input->mMaps)
			{
				map->ResolveBindings();
			}
		}
	}

	mUpdating = true;
	UpdateActionMaps(inputs, GetTime());
	for (auto& user : mUsers)
	{
		user->UpdateRebind();
	}
	mUpdating = false;

	for (auto user : mPendingDestroys)
	{
		ReleaseUser(user);
	}
	mPendingDestroys.clear();
}

InputUser* InputSystem::SyncPlayerInput(PlayerInput& input, PlayerJoinBehavior joinBehavior)
{
	if (input.actions != input.mBuiltActions)
	{
		input.mBuiltActions = input.actions;
		if (input.actions.guid == Guid::InvalidGuid())
		{
			input.Build(InputActionAssetDescriptor());
		}
		else
		{
			const auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
			input.Build(assetManager->LoadDescriptor<InputActionAssetDescriptor>(input.actions));
		}
	}

	InputUser* joined = nullptr;
	input.mUser = GetPlayer(input.playerIndex);
	if (input.mUser == nullptr and joinBehavior == PlayerJoinBehavior::Manual)
	{
		const bool firstPlayer = mPlayers.empty();
		input.mUser = CreatePlayer(input.playerIndex);
		if (firstPlayer)
		{
			for (auto device : mDeviceViews)
			{
				input.mUser->PairDevice(device);
			}
			input.mUser->mPairNewDevices = true;
		}
		else
		{
			GLEAM_CORE_WARN("Player {0} has no paired devices. Add a PlayerInputManager to join additional players.", input.playerIndex);
		}
		joined = input.mUser;
	}
	return joined;
}

void InputSystem::UpdateActionMaps(const TArray<PlayerInput*>& inputs, double time)
{
	struct MapEntry
	{
		WeakPtr<InputActionMap> map;
		InputUser* user = nullptr;
		int32_t priority = 0;
	};

	TArray<MapEntry> maps;
	for (auto input : inputs)
	{
		for (const auto& map : input->mMaps)
		{
			maps.push_back(MapEntry{ .map = map, .user = input->mUser, .priority = map->GetPriority() });
		}
	}

	eastl::stable_sort(maps.begin(), maps.end(), [](const MapEntry& lhs, const MapEntry& rhs)
	{
		return lhs.priority > rhs.priority;
	});

	InputConsumption consumption;
	for (const auto& entry : maps)
	{
		if (const auto map = entry.map.lock())
		{
			map->SetUserActive(entry.user != nullptr and entry.user->IsEnabled() and entry.user->IsRebinding() == false);
			if (map->IsActive())
			{
				map->SyncBindingOverrides(*entry.user);
				map->Update(*entry.user, consumption, time);
			}
		}
	}
}

void InputSystem::ProcessJoins(uint32_t maxPlayers)
{
	for (auto device : mDeviceViews)
	{
		if (mPlayers.size() >= maxPlayers or device->IsConnected() == false or IsPairedToPlayer(device))
		{
			continue;
		}

		InputDevice* const candidates[] = { device };
		InputControl control;
		if (FindActuatedControl(TArrayView<InputDevice* const>(candidates, 1), control) == false)
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

InputUser* InputSystem::CreatePlayer(uint32_t index)
{
	auto player = mUsers.emplace_back(CreateScope<InputUser>(index)).get();
	mPlayers.push_back(player);
	eastl::stable_sort(mPlayers.begin(), mPlayers.end(), [](const InputUser* lhs, const InputUser* rhs)
	{
		return lhs->GetPlayerIndex() < rhs->GetPlayerIndex();
	});
	return player;
}

InputUser* InputSystem::CreateUser()
{
	return mUsers.emplace_back(CreateScope<InputUser>(InputUser::InvalidPlayerIndex)).get();
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
	auto it = eastl::find_if(mUsers.begin(), mUsers.end(), [user](const Scope<InputUser>& candidate)
	{
		return candidate.get() == user;
	});

	if (it == mUsers.end())
	{
		return;
	}

	if (user->GetPlayerIndex() != InputUser::InvalidPlayerIndex)
	{
		EventDispatcher<PlayerLeftEvent>::Publish(PlayerLeftEvent(user));
		mPlayers.erase(eastl::find(mPlayers.begin(), mPlayers.end(), user));
	}

	for (auto input : mPlayerInputs)
	{
		if (input->mUser == user)
		{
			input->mUser = nullptr;
		}
	}

	user->CancelInteractiveRebind();
	mUsers.erase(it);
}

void InputSystem::AddPlayerInput(PlayerInput* input, InputUser* user)
{
	input->mUser = user;
	mPlayerInputs.push_back(input);
}

void InputSystem::RemovePlayerInput(PlayerInput* input)
{
	auto it = eastl::find(mPlayerInputs.begin(), mPlayerInputs.end(), input);
	if (it != mPlayerInputs.end())
	{
		input->mUser = nullptr;
		mPlayerInputs.erase(it);
	}
}

bool InputSystem::IsWorldInputEnabled() const
{
	return mWorldInputEnabled;
}

void InputSystem::SetWorldInputEnabled(bool enabled)
{
	mWorldInputEnabled = enabled;
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
