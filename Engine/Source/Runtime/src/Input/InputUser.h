#pragma once
#include "InputActionAsset.h"

#include <functional>

namespace Gleam {

using InputRebindCallback = std::function<void(bool completed)>;

class InputUser
{
	friend class InputSystem;
public:

	static constexpr uint32_t InvalidPlayerIndex = ~0u;

	InputUser(uint32_t playerIndex);

	uint32_t GetPlayerIndex() const;

	bool IsEnabled() const;

	void SetEnabled(bool enabled);

	void PairDevice(InputDevice* device);

	void UnpairDevice(InputDevice* device);

	void UnpairAllDevices();

	bool IsPaired(const InputDevice* device) const;

	const TArray<InputDevice*>& GetPairedDevices() const;

	void ApplyBindingOverride(const InputActionAsset& asset, InputActionHandle action, uint32_t binding, uint32_t part, const InputControl& control);

	void ClearBindingOverrides();

	const InputBindingOverrides& GetBindingOverrides() const;

	uint32_t GetBindingOverridesVersion() const;

	TString SaveBindingOverrides() const;

	void LoadBindingOverrides(const TString& data);

	void StartInteractiveRebind(const InputActionAsset& asset, InputActionHandle action, uint32_t binding, uint32_t part, InputRebindCallback&& callback);

	void CancelInteractiveRebind();

	bool IsRebinding() const;

private:

	struct RebindOperation
	{
		Guid map;
		Guid action;
		Guid binding;
		uint32_t part = 0;
		InputRebindCallback callback;
		bool armed = false;
	};

	void SetBindingOverride(const InputBindingOverride& entry);

	void UpdateRebind();

	void FinishRebind(bool completed);

	uint32_t mPlayerIndex;

	bool mEnabled = true;

	bool mPairNewDevices = false;

	TArray<InputDevice*> mDevices;

	InputBindingOverrides mOverrides;

	uint32_t mOverridesVersion;

	Optional<RebindOperation> mRebind;

};

} // namespace Gleam
