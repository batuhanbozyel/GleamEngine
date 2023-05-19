#include "SceneViewController.h"

using namespace GEditor;

void SceneViewController::OnCreate(Gleam::EntityManager& entityManager)
{
	mCameraEntity = entityManager.CreateEntity();
    entityManager.AddComponent<Gleam::Camera>(mCameraEntity, GameInstance->GetWindow()->GetResolution());
    
//    const auto& selectedFiles = Gleam::IOUtils::OpenFileDialog(Gleam::FileType::Model);
//    for (const auto& file : selectedFiles)
//    {
//        auto entity = entityManager.CreateEntity();
//        entityManager.AddComponent<Gleam::MeshRenderer>(entity, Gleam::AssetLibrary<Gleam::Model>::Import(file));
//    }

	Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e)
    {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            mCursorVisible = !mCursorVisible;
            Gleam::Input::ShowCursor(mCursorVisible);
        }
    });
    
    Gleam::EventDispatcher<Gleam::WindowResizeEvent>::Subscribe([&](const Gleam::WindowResizeEvent& e)
    {
        auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
        camera.SetViewport(e.GetWidth(), e.GetHeight());
    });
}

void SceneViewController::OnUpdate(Gleam::EntityManager& entityManager)
{
	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCameraEntity);
	ProcessCameraRotation(camera);
	ProcessCameraMovement(camera);

	auto debugRenderer = GameInstance->GetRenderPipeline()->GetRenderer<Gleam::DebugRenderer>();
    debugRenderer->UpdateCamera(camera);
    
    constexpr int gridWidth = 32;
    constexpr int gridHeight = 32;
    Gleam::Color gridColor = Gleam::Color::HSVToRGB(static_cast<float>(Gleam::Time::time), 1.0f, 1.0f);
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
    
    auto worldRenderer = GameInstance->GetRenderPipeline()->GetRenderer<Gleam::WorldRenderer>();
    worldRenderer->UpdateCamera(camera);
    
    entityManager.ForEach<Gleam::MeshRenderer, Gleam::Transform>([&](const Gleam::MeshRenderer& meshRenderer, const Gleam::Transform& transform)
    {
        worldRenderer->DrawMesh(meshRenderer, transform);
    });
}

void SceneViewController::ProcessCameraRotation(Gleam::Camera& camera)
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

void SceneViewController::ProcessCameraMovement(Gleam::Camera& camera)
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
