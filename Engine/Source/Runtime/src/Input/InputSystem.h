#pragma once
#include "Core/Subsystem.h"
#include "Container/Pointer.h"
#include "Devices/Keyboard.h"
#include "Devices/Mouse.h"
#include "PlayerInput.h"
#include "InputActionAsset.h"

union SDL_Event;

namespace Gleam {

enum class PlayerJoinBehavior
{
	Manual,
	JoinOnButtonPress
};

class InputSystem final : public EngineSubsystem
{
	friend class Application;
	friend class EventSystem;
public:

	virtual void Initialize(Engine* engine) override;

	virtual void Shutdown(Engine* engine) override;

	Keyboard* GetKeyboard() const;

	Mouse* GetMouse() const;

	const TArray<Gamepad*>& GetGamepads() const;

	TArrayView<InputDevice* const> GetDevices() const;

	PlayerInput* CreatePlayer();

	PlayerInput* CreateUser();

	void DestroyPlayer(PlayerInput* player);

	void DestroyPlayers();

	PlayerInput* GetPlayer(uint32_t index) const;

	const TArray<PlayerInput*>& GetPlayers() const;

	PlayerJoinBehavior GetJoinBehavior() const;

	void SetJoinBehavior(PlayerJoinBehavior behavior);

	static double GetTime();

	static bool FindActuatedControl(TArrayView<InputDevice* const> devices, InputControl& control);

	void ShowCursor() const;

	void HideCursor() const;

	bool CursorVisible() const;

private:

	void Update();

	bool ProcessEvent(const SDL_Event& e);

	void ConnectGamepad(SDL_JoystickID joystickId);

	void DisconnectGamepad(SDL_JoystickID joystickId);

	Gamepad* FindGamepad(SDL_JoystickID joystickId) const;

	bool IsPairedToPlayer(const InputDevice* device) const;

	void ProcessJoins();

	void UpdateActionMaps(double time);

	uint32_t FindFreePlayerIndex() const;

	TArray<Scope<InputDevice>> mDevices;

	TArray<InputDevice*> mDeviceViews;

	TArray<Scope<PlayerInput>> mUsers;

	TArray<PlayerInput*> mPlayers;

	PlayerJoinBehavior mJoinBehavior = PlayerJoinBehavior::Manual;

	bool mKeymapChanged = false;

	TArray<Gamepad*> mGamepads;

	Keyboard* mKeyboard = nullptr;

	Mouse* mMouse = nullptr;

	mutable bool mCursorHidden = false;

	mutable Float2 mCursorHidePosition = Float2::zero;

};

} // namespace Gleam
