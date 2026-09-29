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
class UndoSystem;

class EntityInspector final : public View
{
public:

	virtual void OnCreate(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawEntities(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawTransform(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawComponents(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawAddComponent(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawSingleton(uint32_t typeHash);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelectionSystem = nullptr;

	UndoSystem* mUndoSystem = nullptr;

};

} // namespace GEditor
