#include "gpch.h"
#include "PlayerInput.h"

using namespace Gleam;

PlayerInput::PlayerInput(const PlayerInput& other)
	: actions(other.actions), playerIndex(other.playerIndex)
{

}

PlayerInput& PlayerInput::operator=(const PlayerInput& other)
{
	actions = other.actions;
	playerIndex = other.playerIndex;
	mMaps.clear();
	mBuiltActions = AssetReference();
	mUser = nullptr;
	return *this;
}

void PlayerInput::Build(const InputActionAssetDescriptor& descriptor)
{
	mMaps.clear();
	mMaps.reserve(descriptor.maps.size());
	for (const auto& map : descriptor.maps)
	{
		mMaps.push_back(CreateRef<InputActionMap>(map));
	}
}

InputUser* PlayerInput::GetUser() const
{
	return mUser;
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

const TArray<RefCounted<InputActionMap>>& PlayerInput::GetActionMaps() const
{
	return mMaps;
}
