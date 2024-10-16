//
//  WorldOutliner.h
//  Editor
//
//  Created by Batuhan Bozyel on 25.05.2023.
//

#pragma once
#include "View/View.h"
#include "Gleam.h"

namespace GEditor {

class WorldOutliner final : public View
{
public:
    
	virtual void Init(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:
    
    void DrawEntityPopupMenu();

	Gleam::World* mEditWorld;
    
    Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
