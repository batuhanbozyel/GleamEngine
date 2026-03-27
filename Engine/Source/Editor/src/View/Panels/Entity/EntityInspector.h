//
//  EntityInspector.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "View/View.h"

namespace GEditor {

class EntityInspector final : public View
{
public:
    
	virtual void OnCreate(Gleam::World* world) override;
    
    virtual void Render(Gleam::ImGuiRenderer* imgui) override;
    
private:

	Gleam::World* mEditWorld;

	uint32_t mSelectedSingletonID = 0;

	Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;
    
};

} // namespace GEditor
