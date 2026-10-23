#include "gpch.h"
#include "PlayerInputSystem.h"
#include "InputSystem.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Assets/AssetManager.h"
#include "World/EntityManager.h"

using namespace Gleam;

void PlayerInputSystem::OnCreate(EntityManager& entityManager)
{
	mInputSystem = Globals::Engine->GetSubsystem<InputSystem>();
	mAssetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	mPlayerInputRemoved = entityManager.OnComponentRemoved<PlayerInput, &PlayerInputSystem::OnPlayerInputRemoved>(*this);
}

void PlayerInputSystem::OnPlayerInputRemoved(EntityHandle entity)
{
	const uint32_t index = FindRecordIndex(entity);
	if (index != InvalidRecordIndex)
	{
		ReleaseActions(mRecords[index]);

		const uint32_t last = static_cast<uint32_t>(mRecords.size()) - 1;
		if (index != last)
		{
			mRecords[index] = eastl::move(mRecords[last]);
			mRecordIndices[entt::to_entity(mRecords[index].entity)] = index;
		}
		mRecords.pop_back();
		mRecordIndices[entt::to_entity(entity)] = InvalidRecordIndex;
	}
}

void PlayerInputSystem::OnDestroy(EntityManager& entityManager)
{
	mPlayerInputRemoved.Reset();
	for (auto& record : mRecords)
	{
		ReleaseActions(record);
	}
	mRecords.clear();
	mRecordIndices.clear();
}

void PlayerInputSystem::OnPreUpdate(EntityManager& entityManager)
{
	SyncRecords(entityManager);

	if (mInputSystem->IsWorldInputEnabled())
	{
		SyncPlayers(entityManager.GetSingleton<PlayerInputManager>());
	}

	for (auto& record : mRecords)
	{
		if (record.states.asset)
		{
			const auto user = mInputSystem->IsWorldInputEnabled() ? mInputSystem->GetPlayer(record.playerIndex) : nullptr;
			mInputSystem->UpdateActions(record.states, user);
		}
	}
}

const InputActionStates* PlayerInputSystem::GetActionStates(EntityHandle entity) const
{
	const uint32_t index = FindRecordIndex(entity);
	if (index != InvalidRecordIndex and mRecords[index].states.asset)
	{
		return &mRecords[index].states;
	}
	else
	{
		return nullptr;
	}
}

void PlayerInputSystem::SetActionMapEnabled(EntityHandle entity, InputActionMapHandle map, bool enabled)
{
	const uint32_t index = FindRecordIndex(entity);
	if (index != InvalidRecordIndex and mRecords[index].states.asset)
	{
		mRecords[index].states.SetActionMapEnabled(map, enabled);
	}
}

void PlayerInputSystem::SyncRecords(EntityManager& entityManager)
{
	const auto& tracker = entityManager.GetChangeTracker();
	if (tracker.GetTick() != mChangeCursor.GetLastSeen())
	{
		const Tick since = mChangeCursor.Begin(tracker);
		entityManager.ForEachChanged<PlayerInput>(since, [&](EntityHandle handle, const PlayerInput& input)
		{
			uint32_t index = FindRecordIndex(handle);
			if (index == InvalidRecordIndex)
			{
				const uint32_t slot = entt::to_entity(handle);
				if (slot >= mRecordIndices.size())
				{
					mRecordIndices.resize(slot + 1, InvalidRecordIndex);
				}

				index = static_cast<uint32_t>(mRecords.size());
				mRecordIndices[slot] = index;
				mRecords.push_back(PlayerInputRecord{ .entity = handle });
			}

			auto& record = mRecords[index];
			if (record.actions != input.actions)
			{
				ReleaseActions(record);
				record.actions = input.actions;
				if (auto asset = mAssetManager->Load<InputActionAsset>(record.actions))
				{
					record.states = InputActionStates(asset);
				}
			}
			record.playerIndex = input.playerIndex;
		});
		mChangeCursor.Commit();
	}
}

void PlayerInputSystem::SyncPlayers(const PlayerInputManager& manager)
{
	if (manager.joinBehavior == PlayerJoinBehavior::JoinOnButtonPress)
	{
		mInputSystem->ProcessJoins(manager.maxPlayers);
	}
	else
	{
		TArray<InputUser*> joined;
		for (const auto& record : mRecords)
		{
			if (record.playerIndex != InputUser::InvalidPlayerIndex and mInputSystem->GetPlayer(record.playerIndex) == nullptr)
			{
				joined.push_back(mInputSystem->JoinPlayer(record.playerIndex));
			}
		}

		for (auto player : joined)
		{
			EventDispatcher<PlayerJoinedEvent>::Publish(PlayerJoinedEvent(player));
		}
	}
}

void PlayerInputSystem::ReleaseActions(PlayerInputRecord& record)
{
	record.states = InputActionStates();
	mAssetManager->Release(record.actions);
}

uint32_t PlayerInputSystem::FindRecordIndex(EntityHandle entity) const
{
	const uint32_t slot = entt::to_entity(entity);
	const uint32_t index = slot < mRecordIndices.size() ? mRecordIndices[slot] : InvalidRecordIndex;
	if (index == InvalidRecordIndex or mRecords[index].entity != entity)
	{
		return InvalidRecordIndex;
	}
	return index;
}
