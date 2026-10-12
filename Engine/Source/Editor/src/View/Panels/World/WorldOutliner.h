//
//  WorldOutliner.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "View/View.h"
#include "World/Entity.h"

#include <imgui.h>

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

class SelectionSystem;
class UndoSystem;
class EWorldManager;
struct EditorFonts;
enum class SelectionMode;

class WorldOutliner final : public View
{
public:

	WorldOutliner(Gleam::World* world);

	virtual void OnCreate(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	struct RowState
	{
		bool clicked = false;
		bool rightClicked = false;
	};

	RowState DrawRow(const char* label, const char* icon, const ImVec4& iconColor, bool selected, bool dimmed, float indent);

	void DrawEntityNode(const Gleam::Entity& entity, uint32_t depth);

	bool MatchesSearch(const Gleam::Entity& entity) const;

	void DrawSingletonComponents();

	void HandleSelectionInput(Gleam::EntityHandle handle);

	void SelectRange(Gleam::EntityHandle anchor, Gleam::EntityHandle target, SelectionMode mode);

	void ToggleActive(Gleam::EntityHandle handle);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelectionSystem = nullptr;

	UndoSystem* mUndoSystem = nullptr;

	EWorldManager* mWorldManager = nullptr;

	const EditorFonts* mFonts = nullptr;

	Gleam::TArray<Gleam::EntityHandle> mVisibleEntities;

	Gleam::EntityHandle mRangeAnchor = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingRangeSelect = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingDestroy = Gleam::InvalidEntity;

	Gleam::EntityHandle mPendingToggleActive = Gleam::InvalidEntity;

	bool mPendingRangeAdditive = false;

	char mSearch[128] = "";

};

} // namespace GEditor
