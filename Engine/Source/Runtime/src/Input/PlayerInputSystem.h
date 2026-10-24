#pragma once
#include "InputActionState.h"
#include "InputUser.h"
#include "World/ComponentSystem.h"
#include "World/Entity.h"

namespace Gleam {

class InputSystem;
class AssetManager;
struct PlayerInputManager;

struct PlayerInputRecord
{
	EntityHandle entity = InvalidEntity;
	AssetReference actions;
	uint32_t playerIndex = InputUser::InvalidPlayerIndex;
	InputActionStates states;
};

class PlayerInputSystem final : public ComponentSystem
{
	static constexpr uint32_t InvalidRecordIndex = ~0u;
public:

	virtual void OnCreate(EntityManager& entityManager) override;

	virtual void OnPreUpdate(EntityManager& entityManager) override;

	virtual void OnDestroy(EntityManager& entityManager) override;

	const InputActionStates* GetActionStates(EntityHandle entity) const;

	void SetActionMapEnabled(EntityHandle entity, InputActionMapHandle map, bool enabled);

private:

	struct MapEntry
	{
		InputActionStates* states = nullptr;
		InputActionMapHandle map;
		const InputUser* user = nullptr;
		int32_t priority = 0;
	};

	void SyncRecords(const EntityManager& entityManager);

	void SyncPlayers(const EntityManager& entityManager);

	void ReleaseActions(PlayerInputRecord& record);

	uint32_t FindRecordIndex(EntityHandle entity) const;

	void OnPlayerInputRemoved(EntityHandle entity);

	InputSystem* mInputSystem = nullptr;

	AssetManager* mAssetManager = nullptr;

	TArray<PlayerInputRecord> mRecords;

	TArray<uint32_t> mRecordIndices;

	TArray<MapEntry> mMapEntries;

	ChangeCursor mChangeCursor;

	ComponentCallback mPlayerInputRemoved;

};

} // namespace Gleam
