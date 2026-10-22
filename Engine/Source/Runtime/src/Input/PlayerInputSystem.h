#pragma once
#include "InputActionSet.h"
#include "World/ComponentSystem.h"
#include "World/Entity.h"
#include "Assets/AssetReference.h"
#include "Container/Pointer.h"

namespace Gleam {

class InputSystem;

struct PlayerInputRecord
{
	EntityHandle entity = InvalidEntity;
	AssetReference actions;
	Scope<InputActionSet> actionSet;
};

class PlayerInputSystem final : public ComponentSystem
{
	static constexpr uint32_t InvalidRecordIndex = ~0u;
public:

	virtual void OnCreate(EntityManager& entityManager) override;

	virtual void OnPreUpdate(EntityManager& entityManager) override;

	virtual void OnDestroy(EntityManager& entityManager) override;

	InputActionSet* GetActionSet(EntityHandle entity) const;

private:

	void SyncRecords(EntityManager& entityManager);

	uint32_t FindRecordIndex(EntityHandle entity) const;

	void OnPlayerInputRemoved(EntityHandle entity);

	InputSystem* mInputSystem = nullptr;

	TArray<PlayerInputRecord> mRecords;

	TArray<uint32_t> mRecordIndices;

	ChangeCursor mChangeCursor;

	ComponentCallback mPlayerInputRemoved;

};

} // namespace Gleam
