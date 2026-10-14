#pragma once
#include "InputActionMap.h"

namespace Gleam {

class InputActionAsset;

using InputRebindCallback = std::function<void(bool completed)>;

class PlayerInput
{
	friend class InputSystem;
public:

	static constexpr uint32_t InvalidPlayerIndex = ~0u;

	PlayerInput(uint32_t playerIndex);

	uint32_t GetPlayerIndex() const;

	bool IsEnabled() const;

	void SetEnabled(bool enabled);

	void PairDevice(InputDevice* device);

	void UnpairDevice(InputDevice* device);

	void UnpairAllDevices();

	bool IsPaired(const InputDevice* device) const;

	const TArray<InputDevice*>& GetPairedDevices() const;

	InputActionMap* AddActionMap(const InputActionMapDescriptor& descriptor, int32_t priority);

	InputActionMap* AddActionMap(const InputActionAsset* asset, TStringView mapName, int32_t priority);

	void RemoveActionMap(InputActionMap* map);

	InputActionMap* FindActionMap(TStringView name) const;

	InputAction* FindAction(TStringView path) const;

	const TArray<Scope<InputActionMap>>& GetActionMaps() const;

	void ApplyBindingOverride(InputAction* action, uint32_t binding, uint32_t part, const InputControl& control);

	void ClearBindingOverrides();

	TString SaveBindingOverrides() const;

	void LoadBindingOverrides(const TString& data);

	void StartInteractiveRebind(InputAction* action, uint32_t binding, uint32_t part, InputRebindCallback&& callback);

	void CancelInteractiveRebind();

	bool IsRebinding() const;

private:

	struct RebindOperation
	{
		InputAction* action = nullptr;
		uint32_t binding = 0;
		uint32_t part = 0;
		InputRebindCallback callback;
		bool armed = false;
	};

	void UpdateRebind();

	void FinishRebind(bool completed);

	void ApplyStoredOverrides(InputActionMap* map);

	uint32_t mPlayerIndex;

	bool mEnabled = true;

	bool mPairNewDevices = false;

	TArray<InputDevice*> mDevices;

	TArray<Scope<InputActionMap>> mMaps;

	InputBindingOverrides mOverrides;

	Optional<RebindOperation> mRebind;

};

} // namespace Gleam
