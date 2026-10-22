#pragma once
#include "Core/Subsystem.h"
#include "Devices/Keyboard.h"
#include "Devices/Mouse.h"
#include "InputUser.h"
#include "InputActionSet.h"
#include "PlayerInput.h"

union SDL_Event;

namespace Gleam {

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

	InputUser* CreateUser();

	void DestroyUser(InputUser* user);

	void DestroyPlayers();

	InputUser* GetPlayer(uint32_t index) const;

	const TArray<InputUser*>& GetPlayers() const;

	void SetPlayerInputManager(const PlayerInputManager& manager);

	void AddActionSet(InputActionSet* actionSet);

	void RemoveActionSet(InputActionSet* actionSet);

	bool IsWorldInputEnabled() const;

	void SetWorldInputEnabled(bool enabled);

	static double GetTime();

	static bool FindActuatedControl(TArrayView<InputDevice* const> devices, InputControl& control);

	void ShowCursor();

	void HideCursor();

	bool CursorVisible() const;

private:

	void Update();

	bool ProcessEvent(const SDL_Event& e);

	void ConnectGamepad(SDL_JoystickID joystickId);

	void DisconnectGamepad(SDL_JoystickID joystickId);

	Gamepad* FindGamepad(SDL_JoystickID joystickId) const;

	bool IsPairedToPlayer(const InputDevice* device) const;

	InputUser* CreatePlayer(uint32_t index);

	void ReleaseUser(InputUser* user);

	void ProcessJoins(uint32_t maxPlayers);

	InputUser* JoinPlayer(uint32_t index);

	void SyncPlayers();

	bool IsUserActive(const InputUser* user) const;

	void UpdateActionMaps(double time);

	uint32_t FindFreePlayerIndex() const;

	TArray<InputDevice*> mDevices;

	TArray<InputUser*> mUsers;

	TArray<InputUser*> mPlayers;

	TArray<InputActionSet*> mActionSets;

	PlayerInputManager mPlayerInputManager;

	TArray<InputUser*> mPendingDestroys;

	bool mUpdating = false;

	bool mWorldInputEnabled = true;

	bool mKeymapChanged = false;

	TArray<Gamepad*> mGamepads;

	Keyboard* mKeyboard = nullptr;

	Mouse* mMouse = nullptr;

	Float2 mCursorHidePosition = Float2::zero;

};

} // namespace Gleam
