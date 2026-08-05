//
//  SelectionSystem.h
//  Editor
//

#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"
#include "World/Systems/PickingSystem.h"

namespace GEditor {

class SelectionSystem final : public Gleam::WorldSubsystem
{
public:

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown(Gleam::World* world) override;

	void SelectEntity(Gleam::EntityHandle entity);

	void SelectSingleton(uint32_t typeHash);

	Gleam::EntityHandle GetSelectedEntity() const
	{
		return mSelectedEntity;
	}

	uint32_t GetSelectedSingleton() const
	{
		return mSelectedSingleton;
	}

private:

	Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;

	uint32_t mSelectedSingleton = 0;

	Gleam::PickingCallbackHandle mPickingCallback = Gleam::InvalidPickingCallback;

};

} // namespace GEditor
