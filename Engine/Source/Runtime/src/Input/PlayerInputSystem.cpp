#include "gpch.h"
#include "PlayerInputSystem.h"
#include "PlayerInput.h"
#include "InputSystem.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Assets/AssetManager.h"
#include "World/EntityManager.h"

using namespace Gleam;

void PlayerInputSystem::OnCreate(EntityManager& entityManager)
{
	mInputSystem = Globals::Engine->GetSubsystem<InputSystem>();
	mPlayerInputRemoved = entityManager.OnComponentRemoved<PlayerInput, &PlayerInputSystem::OnPlayerInputRemoved>(*this);
}

void PlayerInputSystem::OnPlayerInputRemoved(EntityHandle entity)
{
	const uint32_t index = FindRecordIndex(entity);
	if (index != InvalidRecordIndex)
	{
		mInputSystem->RemoveActionSet(mRecords[index].actionSet.get());

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
	for (const auto& record : mRecords)
	{
		mInputSystem->RemoveActionSet(record.actionSet.get());
	}
	mRecords.clear();
	mRecordIndices.clear();
}

void PlayerInputSystem::OnPreUpdate(EntityManager& entityManager)
{
	SyncRecords(entityManager);
	mInputSystem->SetPlayerInputManager(entityManager.GetSingleton<PlayerInputManager>());
}

InputActionSet* PlayerInputSystem::GetActionSet(EntityHandle entity) const
{
	const uint32_t index = FindRecordIndex(entity);
	if (index != InvalidRecordIndex)
	{
		return mRecords[index].actionSet.get();
	}
	else
	{
		return nullptr;
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
				mRecords.push_back(PlayerInputRecord{ .entity = handle, .actionSet = CreateScope<InputActionSet>() });
				mInputSystem->AddActionSet(mRecords[index].actionSet.get());
			}

			auto& record = mRecords[index];
			if (record.actions != input.actions)
			{
				record.actions = input.actions;
				if (input.actions.guid == Guid::InvalidGuid())
				{
					record.actionSet->Build(InputActionAssetDescriptor());
				}
				else
				{
					const auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
					record.actionSet->Build(assetManager->LoadDescriptor<InputActionAssetDescriptor>(input.actions));
				}
			}
			record.actionSet->SetPlayerIndex(input.playerIndex);
		});
		mChangeCursor.Commit();
	}
}

uint32_t PlayerInputSystem::FindRecordIndex(EntityHandle entity) const
{
	const uint32_t slot = entt::to_entity(entity);
	const uint32_t index = slot < mRecordIndices.size() ? mRecordIndices[slot] : InvalidRecordIndex;
	if (index != InvalidRecordIndex and mRecords[index].entity == entity)
	{
		return index;
	}
	else
	{
		return InvalidRecordIndex;
	}
}
