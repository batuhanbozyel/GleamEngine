//
//  WorldViewportController.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "Gleam.h"

namespace GEditor {

class EditorCameraController : public Gleam::ComponentSystem
{
public:

	EditorCameraController(Gleam::EntityHandle cameraEntity);
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
private:
    
	void ProcessCameraMovement(Gleam::Entity& camera);

	void ProcessCameraRotation(Gleam::Entity& camera);

	bool mCursorVisible = true;
    
    bool mViewportFocused = false;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    Gleam::EntityHandle mCameraEntity;
    
};

} // namespace GEditor
