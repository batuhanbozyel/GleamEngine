//
//  EntityInspector.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"
#include "Gleam.h"

namespace GEditor {

class EntityInspector final : public View
{
public:
    
    EntityInspector();
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	Gleam::Entity mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
