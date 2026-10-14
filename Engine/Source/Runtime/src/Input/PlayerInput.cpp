#include "gpch.h"
#include "PlayerInput.h"
#include "InputActionAsset.h"
#include "InputSystem.h"

#include "Serialization/JSONSerializer.h"

using namespace Gleam;

PlayerInput::PlayerInput(uint32_t playerIndex)
	: mPlayerIndex(playerIndex)
{

}

uint32_t PlayerInput::GetPlayerIndex() const
{
	return mPlayerIndex;
}

bool PlayerInput::IsEnabled() const
{
	return mEnabled;
}

void PlayerInput::SetEnabled(bool enabled)
{
	mEnabled = enabled;
	for (auto& map : mMaps)
	{
		map->RefreshActivation();
	}
}

void PlayerInput::PairDevice(InputDevice* device)
{
	if (IsPaired(device) == false)
	{
		mDevices.push_back(device);
	}
}

void PlayerInput::UnpairDevice(InputDevice* device)
{
	auto it = eastl::find(mDevices.begin(), mDevices.end(), device);
	if (it != mDevices.end())
	{
		mDevices.erase(it);
	}
}

void PlayerInput::UnpairAllDevices()
{
	mDevices.clear();
	mPairNewDevices = false;
}

bool PlayerInput::IsPaired(const InputDevice* device) const
{
	return eastl::find(mDevices.begin(), mDevices.end(), device) != mDevices.end();
}

const TArray<InputDevice*>& PlayerInput::GetPairedDevices() const
{
	return mDevices;
}

InputActionMap* PlayerInput::AddActionMap(const InputActionMapDescriptor& descriptor, int32_t priority)
{
	auto map = mMaps.emplace_back(CreateScope<InputActionMap>(this, descriptor, priority)).get();
	ApplyStoredOverrides(map);
	map->RefreshActivation();
	return map;
}

InputActionMap* PlayerInput::AddActionMap(const InputActionAsset* asset, TStringView mapName, int32_t priority)
{
	const auto descriptor = asset->FindActionMap(mapName);
	if (descriptor == nullptr)
	{
		GLEAM_CORE_ERROR("Input action map {0} could not be found in {1}", std::string_view(mapName.data(), mapName.size()), asset->GetDescriptor().name);
		return nullptr;
	}
	return AddActionMap(*descriptor, priority);
}

void PlayerInput::RemoveActionMap(InputActionMap* map)
{
	auto it = eastl::find_if(mMaps.begin(), mMaps.end(), [map](const Scope<InputActionMap>& candidate)
	{
		return candidate.get() == map;
	});

	if (it == mMaps.end())
	{
		return;
	}

	if (mRebind and mRebind->action->GetMap() == map)
	{
		FinishRebind(false);
	}

	map->SetEnabled(false);
	mMaps.erase(it);
}

InputActionMap* PlayerInput::FindActionMap(TStringView name) const
{
	for (const auto& map : mMaps)
	{
		if (map->GetName() == name)
		{
			return map.get();
		}
	}
	return nullptr;
}

InputAction* PlayerInput::FindAction(TStringView path) const
{
	const auto separator = path.find('/');
	if (separator != TStringView::npos)
	{
		const auto map = FindActionMap(path.substr(0, separator));
		return map ? map->FindAction(path.substr(separator + 1)) : nullptr;
	}

	for (const auto& map : mMaps)
	{
		if (auto action = map->FindAction(path))
		{
			return action;
		}
	}
	return nullptr;
}

const TArray<Scope<InputActionMap>>& PlayerInput::GetActionMaps() const
{
	return mMaps;
}

void PlayerInput::ApplyBindingOverride(InputAction* action, uint32_t binding, uint32_t part, const InputControl& control)
{
	const auto& mapId = action->GetMap()->GetId();
	auto it = eastl::find_if(mOverrides.overrides.begin(), mOverrides.overrides.end(), [&](const InputBindingOverride& entry)
	{
		return entry.map == mapId and entry.action == action->GetId() and entry.binding == binding and entry.part == part;
	});

	if (it != mOverrides.overrides.end())
	{
		it->control = control;
	}
	else
	{
		mOverrides.overrides.push_back(InputBindingOverride{
			.map = mapId,
			.action = action->GetId(),
			.binding = binding,
			.part = part,
			.control = control
		});
	}
	action->ApplyOverride(binding, part, control);
}

void PlayerInput::ClearBindingOverrides()
{
	mOverrides.overrides.clear();
	for (auto& map : mMaps)
	{
		for (auto& action : map->mActions)
		{
			action->ResetBindings();
		}
	}
}

TString PlayerInput::SaveBindingOverrides() const
{
	JSONSerializer serializer;
	return serializer.Serialize(&mOverrides, Reflection::GetClass<InputBindingOverrides>());
}

void PlayerInput::LoadBindingOverrides(const TString& data)
{
	ClearBindingOverrides();

	JSONSerializer serializer;
	serializer.Deserialize(Reflection::GetClass<InputBindingOverrides>(), &mOverrides, data);
	for (auto& map : mMaps)
	{
		ApplyStoredOverrides(map.get());
	}
}

void PlayerInput::StartInteractiveRebind(InputAction* action, uint32_t binding, uint32_t part, InputRebindCallback&& callback)
{
	if (mRebind)
	{
		FinishRebind(false);
	}

	mRebind = RebindOperation{
		.action = action,
		.binding = binding,
		.part = part,
		.callback = eastl::move(callback)
	};
}

void PlayerInput::CancelInteractiveRebind()
{
	if (mRebind)
	{
		FinishRebind(false);
	}
}

bool PlayerInput::IsRebinding() const
{
	return mRebind.has_value();
}

void PlayerInput::UpdateRebind()
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
			ApplyBindingOverride(mRebind->action, mRebind->binding, mRebind->part, control);
			FinishRebind(true);
		}
	}
}

void PlayerInput::FinishRebind(bool completed)
{
	auto callback = eastl::move(mRebind->callback);
	mRebind.reset();
	if (callback)
	{
		callback(completed);
	}
}

void PlayerInput::ApplyStoredOverrides(InputActionMap* map)
{
	for (const auto& entry : mOverrides.overrides)
	{
		if (entry.map != map->GetId())
		{
			continue;
		}

		for (auto& action : map->mActions)
		{
			if (action->GetId() == entry.action)
			{
				action->ApplyOverride(entry.binding, entry.part, entry.control);
			}
		}
	}
}
