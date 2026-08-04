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

	Gleam::World* mEditWorld = nullptr;

	uint32_t mSelectedSingletonID = 0;

	Gleam::Float3 mEntityEulerRotation = {};

	// Rotation this panel last wrote, anything else means the entity was rotated elsewhere
	Gleam::Quaternion mEntityRotation = Gleam::Quaternion::identity;

	Gleam::EntityHandle mSelectedEntity = Gleam::InvalidEntity;

};

} // namespace GEditor
