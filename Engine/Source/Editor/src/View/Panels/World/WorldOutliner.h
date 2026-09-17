//
//  WorldOutliner.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "View/View.h"
#include "World/Entity.h"

namespace GEditor {

class SelectionSystem;
class UndoSystem;
enum class SelectionMode;

class WorldOutliner final : public View
{
public:

	virtual void OnCreate(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawEntityNode(const Gleam::Entity& entity);

	void DrawSingletonComponents();

	void HandleSelectionInput(Gleam::EntityHandle handle);

	void SelectRange(Gleam::EntityHandle anchor, Gleam::EntityHandle target, SelectionMode mode);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelectionSystem = nullptr;

	UndoSystem* mUndoSystem = nullptr;

	Gleam::TArray<Gleam::EntityHandle> mVisibleEntities;

	Gleam::EntityHandle mRangeAnchor = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingRangeSelect = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingDestroy = Gleam::InvalidEntity;

	bool mPendingRangeAdditive = false;

};

} // namespace GEditor
