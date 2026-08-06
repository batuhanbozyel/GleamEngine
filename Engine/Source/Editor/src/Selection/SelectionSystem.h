//
//  SelectionSystem.h
//  Editor
//

#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "World/Systems/PickingSystem.h"

namespace GEditor {

class SelectionSystem final : public Gleam::TickableWorldSubsystem
{
public:

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown(Gleam::World* world) override;

	virtual void Tick(Gleam::World* world) override;

	void SelectEntity(Gleam::EntityHandle entity);

	void SelectEntities(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void SelectSingleton(uint32_t typeHash);

	void ClearSelection();

	bool IsSelected(Gleam::EntityHandle entity) const;

	Gleam::EntityHandle GetSelectedEntity() const
	{
		return mSelectedEntities.empty() ? Gleam::InvalidEntity : mSelectedEntities.front();
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

	Gleam::TArray<Gleam::EntityHandle> mSelectedEntities;

	Gleam::TArray<uint32_t> mInstanceMask;

	uint32_t mSelectedSingleton = 0;

	Gleam::PickingCallbackHandle mPickingCallback = Gleam::InvalidPickingCallback;

};

} // namespace GEditor
