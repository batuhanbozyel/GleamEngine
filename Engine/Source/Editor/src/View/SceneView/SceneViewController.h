//
//  SceneViewController.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "Gleam.h"

namespace GEditor {

class EditorSceneViewController : public Gleam::ComponentSystem
{
public:
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
private:
    
	void ProcessCameraMovement(Gleam::Camera& camera);

	void ProcessCameraRotation(Gleam::Camera& camera);

	bool mCursorVisible = true;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    Gleam::Entity mCameraEntity;
    
};

} // namespace GEditor
