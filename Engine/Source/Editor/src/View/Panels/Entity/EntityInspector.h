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

#include <imgui.h>

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

class SelectionSystem;
class UndoSystem;
struct EditorFonts;

class EntityInspector final : public View
{
public:

	EntityInspector(Gleam::World* world);

	virtual void OnCreate(Gleam::Application* app) override;

    virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawHeader(const char* name, const char* kind, const ImVec4& color);

	void DrawEntities(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawTransform(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawComponents(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawAddComponent(const Gleam::TArray<Gleam::EntityHandle>& entities);

	void DrawSingleton(uint32_t typeHash);

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelectionSystem = nullptr;

	UndoSystem* mUndoSystem = nullptr;

	const EditorFonts* mFonts = nullptr;

};

} // namespace GEditor
