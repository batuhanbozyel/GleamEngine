//
//  EntityInspector.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"
#include "World/Entity.h"
#include "Container/Hash.h"

namespace GEditor {

class SelectionSystem;

class EntityInspector final : public View
{
public:

	virtual void OnCreate(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	struct SharedComponent
	{
		const Gleam::Reflection::ClassDescription* classDesc = nullptr;
		Gleam::TArray<void*> instances;
	};

	void DrawEntities(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawTransform(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawComponents(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawSingleton(uint32_t typeHash);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelection = nullptr;

	// Selection with the active entity first, everything else follows its edits
	Gleam::TArray<Gleam::EntityHandle> mSelectionOrder;

	// Component types every selected entity has in common
	Gleam::TArray<SharedComponent> mSharedComponents;

	Gleam::HashMap<uint32_t, void*> mComponentLookup;

	Gleam::Float3 mEntityEulerRotation = {};

	// Rotation this panel last wrote, anything else means the entity was rotated elsewhere
	Gleam::Quaternion mEntityRotation = Gleam::Quaternion::identity;

	// Entity the euler cache was built from
	Gleam::EntityHandle mCachedEntity = Gleam::InvalidEntity;

};

} // namespace GEditor
