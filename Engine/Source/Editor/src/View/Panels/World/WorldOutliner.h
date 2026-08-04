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

class WorldOutliner final : public View
{
public:
    
	virtual void OnCreate(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	void DrawEntityNode(Gleam::EntityHandle handle);

	void DrawSingletonComponents();

	Gleam::World* mEditWorld = nullptr;

	SelectionSystem* mSelection = nullptr;

};

} // namespace GEditor
