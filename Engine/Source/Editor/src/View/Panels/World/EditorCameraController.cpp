//
//  WorldViewportController.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "EditorCameraController.h"

using namespace GEditor;

void EditorCameraController::OnCreate(Gleam::EntityManager& entityManager)
{
	mCameraEntity = entityManager.CreateEntity(Gleam::Guid::NewGuid());
    entityManager.AddComponent<Gleam::Camera>(mCameraEntity, Gleam::Globals::Engine->GetResolution());
}

void EditorCameraController::OnUpdate(Gleam::EntityManager& entityManager)
{
	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
    camera.SetViewport(mViewportSize);
    
	ProcessCameraRotation(camera);
	ProcessCameraMovement(camera);
}

void EditorCameraController::ProcessCameraRotation(Gleam::Camera& camera)
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

void EditorCameraController::ProcessCameraMovement(Gleam::Camera& camera)
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

void EditorCameraController::Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size)
{
    mViewportSize = size;

	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
	camera.SetViewport(mViewportSize);
}
