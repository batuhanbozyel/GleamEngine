#include "SceneViewController.h"

using namespace GEditor;

void EditorSceneViewController::OnCreate(Gleam::EntityManager& entityManager)
{
	mCameraEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Gleam::Camera>(mCameraEntity, GameInstance->GetActiveWindow().GetResolution());

	Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e) -> bool
    {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            mCursorVisible = !mCursorVisible;
            Gleam::Input::ShowCursor(mCursorVisible);
        }
        return false;
    });
}

void EditorSceneViewController::OnUpdate(Gleam::EntityManager& entityManager)
{
	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
	ProcessCameraRotation(camera);
	ProcessCameraMovement(camera);

	auto debugRenderer = Gleam::RenderPipeline::Get()->GetRenderer<Gleam::DebugRenderer>();
    debugRenderer->UpdateCamera(camera);
    
    constexpr int gridWidth = 32;
    constexpr int gridHeight = 32;
    for (int i = 0; i < gridWidth; i++)
    {
        for (int j = 0; j < gridHeight; j++)
        {
            debugRenderer->DrawQuad({float(i - gridWidth / 2), 0.0f, float(j - gridHeight / 2)}, 1.0f, 1.0f, Gleam::Color::HSVToRGB(static_cast<float>(Gleam::Time::time), 1.0f, 1.0f));
        }
    }
}

void EditorSceneViewController::ProcessCameraRotation(Gleam::Camera& camera)
{
	if (!Gleam::Input::GetCursorVisible())
    {
        constexpr float mouseSensitivity = 0.1f;
        mYaw += Gleam::Input::GetAxisX() * mouseSensitivity;
        mPitch += Gleam::Input::GetAxisY() * mouseSensitivity;
        mPitch = Gleam::Math::Clamp(mPitch, -80.0f, 80.0f);
        camera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Vector3{ mPitch, mYaw, 0.0f })));
    }
}

void EditorSceneViewController::ProcessCameraMovement(Gleam::Camera& camera)
{
    constexpr float cameraSpeed = 5.0f;
    float deltaTime = static_cast<float>(Gleam::Time::deltaTime);
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::A))
    {
        camera.Translate(-camera.RightVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::D))
    {
        camera.Translate(camera.RightVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::W))
    {
        camera.Translate(camera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::S))
    {
        camera.Translate(-camera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::Space))
    {
        camera.Translate(Gleam::Vector3::up * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftControl))
    {
        camera.Translate(Gleam::Vector3::down * cameraSpeed * deltaTime);
    }
}