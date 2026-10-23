#include "gpch.h"
#include "InputUser.h"
#include "InputSystem.h"

#include "Serialization/JSONSerializer.h"

using namespace Gleam;

static uint32_t NextOverridesVersion()
{
	static uint32_t version = 0;
	return ++version;
}

InputUser::InputUser(uint32_t playerIndex)
	: mPlayerIndex(playerIndex)
	, mOverridesVersion(NextOverridesVersion())
{

}

uint32_t InputUser::GetPlayerIndex() const
{
	return mPlayerIndex;
}

bool InputUser::IsEnabled() const
{
	return mEnabled;
}

void InputUser::SetEnabled(bool enabled)
{
	mEnabled = enabled;
}

void InputUser::PairDevice(InputDevice* device)
{
	if (IsPaired(device) == false)
	{
		mDevices.push_back(device);
	}
}

void InputUser::UnpairDevice(InputDevice* device)
{
	auto it = eastl::find(mDevices.begin(), mDevices.end(), device);
	if (it != mDevices.end())
	{
		mDevices.erase(it);
	}
}

void InputUser::UnpairAllDevices()
{
	mDevices.clear();
	mPairNewDevices = false;
}

bool InputUser::IsPaired(const InputDevice* device) const
{
	return eastl::find(mDevices.begin(), mDevices.end(), device) != mDevices.end();
}

const TArray<InputDevice*>& InputUser::GetPairedDevices() const
{
	return mDevices;
}

void InputUser::ApplyBindingOverride(const InputActionAsset& asset, InputActionHandle action, uint32_t binding, uint32_t part, const InputControl& control)
{
	SetBindingOverride(InputBindingOverride{
		.map = asset.GetActionMap(asset.GetActionLayout(action).map).id,
		.action = asset.GetAction(action).id,
		.binding = binding,
		.part = part,
		.control = control
	});
}

void InputUser::ClearBindingOverrides()
{
	mOverrides.overrides.clear();
	mOverridesVersion = NextOverridesVersion();
}

const InputBindingOverrides& InputUser::GetBindingOverrides() const
{
	return mOverrides;
}

uint32_t InputUser::GetBindingOverridesVersion() const
{
	return mOverridesVersion;
}

TString InputUser::SaveBindingOverrides() const
{
	JSONSerializer serializer;
	return serializer.Serialize(&mOverrides, Reflection::GetClass<InputBindingOverrides>());
}

void InputUser::LoadBindingOverrides(const TString& data)
{
	mOverrides.overrides.clear();

	JSONSerializer serializer;
	serializer.Deserialize(Reflection::GetClass<InputBindingOverrides>(), &mOverrides, data);
	mOverridesVersion = NextOverridesVersion();
}

void InputUser::StartInteractiveRebind(const InputActionAsset& asset, InputActionHandle action, uint32_t binding, uint32_t part, InputRebindCallback&& callback)
{
	if (mRebind)
	{
		FinishRebind(false);
	}

	mRebind = RebindOperation{
		.map = asset.GetActionMap(asset.GetActionLayout(action).map).id,
		.action = asset.GetAction(action).id,
		.binding = binding,
		.part = part,
		.callback = eastl::move(callback)
	};
}

void InputUser::CancelInteractiveRebind()
{
	if (mRebind)
	{
		FinishRebind(false);
	}
}

bool InputUser::IsRebinding() const
{
	return mRebind.has_value();
}

void InputUser::SetBindingOverride(const InputBindingOverride& entry)
{
	auto it = eastl::find_if(mOverrides.overrides.begin(), mOverrides.overrides.end(), [&](const InputBindingOverride& candidate)
	{
		return candidate.map == entry.map and candidate.action == entry.action and candidate.binding == entry.binding and candidate.part == entry.part;
	});

	if (it != mOverrides.overrides.end())
	{
		it->control = entry.control;
	}
	else
	{
		mOverrides.overrides.push_back(entry);
	}
	mOverridesVersion = NextOverridesVersion();
}

void InputUser::UpdateRebind()
{
	if (mRebind.has_value() == false)
	{
		return;
	}
	else if (mRebind->armed == false)
	{
		mRebind->armed = true;
		return;
	}

	InputControl control;
	if (InputSystem::FindActuatedControl(TArrayView<InputDevice* const>(mDevices.data(), mDevices.size()), control))
	{
		if (control == InputControl::Make(Key::Escape))
		{
			FinishRebind(false);
		}
		else
		{
			SetBindingOverride(InputBindingOverride{
				.map = mRebind->map,
				.action = mRebind->action,
				.binding = mRebind->binding,
				.part = mRebind->part,
				.control = control
			});
			FinishRebind(true);
		}
	}
}

void InputUser::FinishRebind(bool completed)
{
	auto callback = eastl::move(mRebind->callback);
	mRebind.reset();
	if (callback)
	{
		callback(completed);
	}
}
