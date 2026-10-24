#pragma once
#include "Core/Subsystem.h"
#include "Devices/Keyboard.h"
#include "Devices/Mouse.h"
#include "InputUser.h"
#include "InputActionState.h"

#include "Container/Hash.h"

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

	InputUser* JoinPlayer(uint32_t index);

	void ProcessJoins(uint32_t maxPlayers);

	void UpdateActions(InputActionStates& states, const InputUser* user);

	bool PrepareActions(InputActionStates& states, const InputUser* user);

	void UpdateActionMap(InputActionStates& states, InputActionMapHandle map, const InputUser& user, double time);

	bool IsWorldInputEnabled() const;

	void SetWorldInputEnabled(bool enabled);

	double GetTime();

	void ShowCursor();

	void HideCursor();

	bool CursorVisible() const;

private:

	struct ControlSample
	{
		Float3 value = Float3::zero;
		float magnitude = 0.0f;
		bool pressed = false;
		bool released = false;
		const InputDevice* device = nullptr;
	};

	void Update();

	bool ProcessEvent(const SDL_Event& e);

	void ConnectGamepad(SDL_JoystickID joystickId);

	void DisconnectGamepad(SDL_JoystickID joystickId);

	Gamepad* FindGamepad(SDL_JoystickID joystickId) const;

	bool IsPairedToPlayer(const InputDevice* device) const;

	InputUser* CreatePlayer(uint32_t index);

	void ReleaseUser(InputUser* user);

	uint32_t FindFreePlayerIndex() const;

	void ResolveBindings(InputActionStates& states, const InputUser& user) const;

	InputBindingState EvaluateBinding(const InputBindingDescriptor& binding, const InputActionDescriptor& action, const Guid& map, const TArray<InputDevice*>& devices);

	ControlSample SampleControl(const InputControl& control, const Guid& map, const TArray<InputDevice*>& devices) const;

	void ConsumeControl(const InputControl& control, const Guid& map, const TArray<InputDevice*>& devices);

	TArray<InputDevice*> mDevices;

	TArray<InputUser*> mUsers;

	TArray<InputUser*> mPlayers;

	TArray<InputUser*> mPendingDestroys;

	HashMap<uint64_t, Guid> mConsumedControls;

	HashSet<uint64_t> mMapConsumedControls;

	uint32_t mKeymapVersion = 0;

	bool mUpdating = false;

	bool mWorldInputEnabled = true;

	TArray<Gamepad*> mGamepads;

	Keyboard* mKeyboard = nullptr;

	Mouse* mMouse = nullptr;

	Float2 mCursorHidePosition = Float2::zero;

};

} // namespace Gleam
