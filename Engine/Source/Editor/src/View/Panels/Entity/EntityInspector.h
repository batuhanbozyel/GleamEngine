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
    
	virtual void Init(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	Gleam::World* mEditWorld;

	Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
