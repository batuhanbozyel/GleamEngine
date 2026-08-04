//
//  SelectionSystem.h
//  Editor
//

#pragma once
#include "World/WorldSubsystem.h"
#include "World/Entity.h"

namespace GEditor {

// Owns what the editor has selected, an entity and a singleton component selection are mutually exclusive
class SelectionSystem final : public Gleam::WorldSubsystem
{
public:

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

};

} // namespace GEditor
