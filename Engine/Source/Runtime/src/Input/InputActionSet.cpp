#include "gpch.h"
#include "InputActionSet.h"

using namespace Gleam;

void InputActionSet::Build(const InputActionAssetDescriptor& descriptor)
{
	mMaps.clear();
	mMaps.reserve(descriptor.maps.size());
	for (const auto& map : descriptor.maps)
	{
		mMaps.push_back(CreateRef<InputActionMap>(map));
	}
}

InputUser* InputActionSet::GetUser() const
{
	return mUser;
}

void InputActionSet::SetUser(InputUser* user)
{
	mUser = user;
}

uint32_t InputActionSet::GetPlayerIndex() const
{
	return mPlayerIndex;
}

void InputActionSet::SetPlayerIndex(uint32_t index)
{
	mPlayerIndex = index;
}

InputActionMap* InputActionSet::FindActionMap(TStringView name) const
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

InputAction* InputActionSet::FindAction(TStringView path) const
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

const TArray<RefCounted<InputActionMap>>& InputActionSet::GetActionMaps() const
{
	return mMaps;
}
