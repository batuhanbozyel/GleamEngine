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
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
    void Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size);
    
private:
    
	void ProcessCameraMovement(Gleam::Camera& camera);

	void ProcessCameraRotation(Gleam::Camera& camera);

	bool mCursorVisible = true;
    
    bool mViewportFocused = false;
    
    Gleam::Size mViewportSize = Gleam::Size::zero;
    
    float mYaw = 0.0f;
    float mPitch = 0.0f;

    Gleam::Entity mCameraEntity;
    
};

} // namespace GEditor
