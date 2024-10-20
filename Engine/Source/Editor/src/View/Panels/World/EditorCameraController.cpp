//
//  WorldViewportController.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "EditorCameraController.h"

using namespace GEditor;

EditorCameraController::EditorCameraController(Gleam::EntityHandle cameraEntity)
	: mCameraEntity(cameraEntity)
{

}

void EditorCameraController::OnCreate(Gleam::EntityManager& entityManager)
{
	
}

void EditorCameraController::OnUpdate(Gleam::EntityManager& entityManager)
{
	auto& cameraEntity = entityManager.GetComponent<Gleam::Entity>(mCameraEntity);
	ProcessCameraRotation(cameraEntity);
	ProcessCameraMovement(cameraEntity);
}

void EditorCameraController::ProcessCameraRotation(Gleam::Entity& camera)
{
    auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
	if (!inputSystem->CursorVisible())
    {
        constexpr float mouseSensitivity = 0.1f;
        const auto& axis = inputSystem->GetAxis();
        mYaw += axis.x * mouseSensitivity;
        mPitch += axis.y * mouseSensitivity;
        mPitch = Gleam::Math::Clamp(mPitch, -80.0f, 80.0f);
		camera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Float3{ mPitch, mYaw, 0.0f })));
    }
}

void EditorCameraController::ProcessCameraMovement(Gleam::Entity& camera)
{
    constexpr float cameraSpeed = 5.0f;
    auto deltaTime = static_cast<float>(Gleam::Time::deltaTime);
    auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
    if (inputSystem->GetButtonDown(Gleam::KeyCode::A))
    {
		camera.Translate(-camera.RightVector() * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::D))
    {
		camera.Translate(camera.RightVector() * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::W))
    {
		camera.Translate(camera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::S))
    {
		camera.Translate(-camera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::Space))
    {
		camera.Translate(Gleam::Float3::up * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::LeftControl))
    {
		camera.Translate(Gleam::Float3::down * cameraSpeed * deltaTime);
    }
}
