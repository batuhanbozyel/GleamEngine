//
//  WorldViewportController.h
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#pragma once
#include "World/Entity.h"
#include "World/ComponentSystem.h"

namespace Gleam {
class EntityManager;
struct EditorCamera;
} // namespace Gleam

namespace GEditor {

class EditorCameraController : public Gleam::ComponentSystem
{
public:

	EditorCameraController(Gleam::EntityHandle cameraEntity);
    
    virtual void OnCreate(Gleam::EntityManager& entityManager) override;

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;
    
private:
    
	void ProcessCameraMovement(Gleam::Entity& camera);

	void ProcessCameraRotation(Gleam::Entity& camera, Gleam::EditorCamera& editorCamera);

	bool mCursorVisible = true;
    bool mViewportFocused = false;

    Gleam::EntityHandle mCameraEntity;
    
};

} // namespace GEditor
