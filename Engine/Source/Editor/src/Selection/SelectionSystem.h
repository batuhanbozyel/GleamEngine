//
//  SelectionSystem.h
//  Editor
//

#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "World/Systems/PickingSystem.h"
#include "Container/Hash.h"

namespace GEditor {

enum class SelectionMode
{
	Replace,
	Add,
	Toggle,
};

class SelectionSystem final : public Gleam::TickableWorldSubsystem
{
public:

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown(Gleam::World* world) override;

	virtual void Tick(Gleam::World* world) override;

	Gleam::PickingRequestID RequestPick(const Gleam::PickingRequest& request, SelectionMode mode);

	void SelectEntity(Gleam::EntityHandle entity, SelectionMode mode = SelectionMode::Replace);

	void SelectEntities(const Gleam::TArray<Gleam::EntityHandle>& entities, SelectionMode mode = SelectionMode::Replace);

	void SetActiveEntity(Gleam::EntityHandle entity);

	void SelectSingleton(uint32_t typeHash);

	void ClearSelection();

	bool IsSelected(Gleam::EntityHandle entity) const;

	Gleam::EntityHandle GetActiveEntity() const
	{
		return mActiveEntity;
	}

	const Gleam::TArray<Gleam::EntityHandle>& GetSelectedEntities() const
	{
		return mSelectedEntities;
	}

	const Gleam::TArray<uint32_t>& GetInstanceMask() const
	{
		return mInstanceMask;
	}

	uint32_t GetSelectedSingleton() const
	{
		return mSelectedSingleton;
	}

private:

	void ApplyEntity(Gleam::EntityHandle entity, SelectionMode mode);

	void ResolveActiveEntity();

	Gleam::TArray<Gleam::EntityHandle> mSelectedEntities;

	Gleam::EntityHandle mActiveEntity = Gleam::InvalidEntity;

	Gleam::TArray<uint32_t> mInstanceMask;

	uint32_t mSelectedSingleton = 0;

	Gleam::PickingSystem* mPickingSystem = nullptr;

	Gleam::HashMap<Gleam::PickingRequestID, SelectionMode> mPendingPickModes;

	Gleam::PickingCallbackHandle mPickingCallback = Gleam::InvalidPickingCallback;

};

} // namespace GEditor
