//
//  WorldViewportController.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewportController.h"

using namespace GEditor;

void WorldViewportController::OnCreate(Gleam::EntityManager& entityManager)
{
	mCameraEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Gleam::Camera>(mCameraEntity, GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetResolution());
}

void WorldViewportController::OnUpdate(Gleam::EntityManager& entityManager)
{
	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
    camera.SetViewport(mViewportSize);
    
    if (mViewportFocused)
    {
        ProcessCameraRotation(camera);
        ProcessCameraMovement(camera);
    }

	auto debugRenderer = GameInstance->GetSubsystem<Gleam::RenderSystem>()->GetRenderer<Gleam::DebugRenderer>();
    debugRenderer->UpdateCamera(camera);
    
    constexpr int gridWidth = 32;
    constexpr int gridHeight = 32;
    Gleam::Color gridColor = Gleam::Color::HSVToRGB(static_cast<float>(Gleam::Time::elapsedTime), 1.0f, 1.0f);
    for (int i = 0; i < gridWidth; i++)
    {
        for (int j = 0; j < gridHeight; j++)
        {
            constexpr float tileWidth = 1.0f;
            constexpr float tileHeight = 1.0f;
            Gleam::Vector3 tilePosition = {float(i - gridWidth / 2), 0.0f, float(j - gridHeight / 2)};
            debugRenderer->DrawQuad(tilePosition, tileWidth, tileHeight, gridColor, true);
        }
    }
    
    auto worldRenderer = GameInstance->GetSubsystem<Gleam::RenderSystem>()->GetRenderer<Gleam::WorldRenderer>();
    worldRenderer->UpdateCamera(camera);
    
    entityManager.ForEach<Gleam::MeshRenderer, Gleam::Transform>([&](const Gleam::MeshRenderer& meshRenderer, const Gleam::Transform& transform)
    {
        worldRenderer->DrawMesh(meshRenderer, transform);
    });
}

void WorldViewportController::ProcessCameraRotation(Gleam::Camera& camera)
{
    auto inputSystem = GameInstance->GetSubsystem<Gleam::InputSystem>();
	if (!inputSystem->CursorVisible())
    {
        constexpr float mouseSensitivity = 0.1f;
        const auto& axis = inputSystem->GetAxis();
        mYaw += axis.x * mouseSensitivity;
        mPitch += axis.y * mouseSensitivity;
        mPitch = Gleam::Math::Clamp(mPitch, -80.0f, 80.0f);
        camera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Vector3{ mPitch, mYaw, 0.0f })));
    }
}

void WorldViewportController::ProcessCameraMovement(Gleam::Camera& camera)
{
    constexpr float cameraSpeed = 5.0f;
    auto deltaTime = static_cast<float>(Gleam::Time::deltaTime);
    auto inputSystem = GameInstance->GetSubsystem<Gleam::InputSystem>();
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
        camera.Translate(Gleam::Vector3::up * cameraSpeed * deltaTime);
    }
    if (inputSystem->GetButtonDown(Gleam::KeyCode::LeftControl))
    {
        camera.Translate(Gleam::Vector3::down * cameraSpeed * deltaTime);
    }
}

const Gleam::Size& WorldViewportController::GetViewportSize() const
{
    return mViewportSize;
}

void WorldViewportController::SetViewportSize(const Gleam::Size& size)
{
    mViewportSize = size;
}

void WorldViewportController::SetViewportFocused(bool focused)
{
    mViewportFocused = focused;
}
