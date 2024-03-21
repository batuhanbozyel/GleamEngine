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
    
    WorldOutliner();
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:
    
    void DrawEntityPopupMenu();
    
    Gleam::Entity mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
