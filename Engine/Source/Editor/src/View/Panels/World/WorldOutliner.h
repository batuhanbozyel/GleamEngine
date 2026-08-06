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
enum class SelectionMode;

class WorldOutliner final : public View
{
public:

	virtual void OnCreate(Gleam::World* world) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawEntityNode(Gleam::EntityHandle handle);

	void DrawSingletonComponents();

	void HandleSelectionInput(Gleam::EntityHandle handle);

	void SelectRange(Gleam::EntityHandle anchor, Gleam::EntityHandle target, SelectionMode mode);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelection = nullptr;

	// Rows drawn this frame in tree order, a shift click range is resolved against it
	Gleam::TArray<Gleam::EntityHandle> mVisibleEntities;

	Gleam::EntityHandle mRangeAnchor = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingRangeSelect = Gleam::InvalidEntity;

	bool mPendingRangeAdditive = false;

};

} // namespace GEditor
