//
//  WorldOutliner.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "View/View.h"

namespace GEditor {

class WorldOutliner final : public View
{
public:
    
	virtual void Init(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	void DrawEntityNode(Gleam::EntityHandle handle);

	void DrawSingletonComponents();

	Gleam::World* mEditWorld;

	uint32_t mSelectedSingletonID = 0;
    
    Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
