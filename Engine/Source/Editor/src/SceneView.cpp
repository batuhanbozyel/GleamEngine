#include "SceneView.h"

using namespace GEditor;

void SceneView::OnAttach()
{
    mCamera.Translate({0.0f, 0.5f, 0.0f});
    
    Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e) -> bool
    {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            mCursorVisible = !mCursorVisible;
            Gleam::Input::ShowCursor(mCursorVisible);
        }
        return false;
    });

    Gleam::EventDispatcher<Gleam::RendererResizeEvent>::Subscribe([&](const Gleam::RendererResizeEvent& e) -> bool
    {
        mCamera.SetViewport(static_cast<float>(e.GetWidth()), static_cast<float>(e.GetHeight()));
        return false;
    });
}

void SceneView::OnUpdate()
{
#if defined(PLATFORM_WINDOWS)
    if ((Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftAlt) || Gleam::Input::GetButtonDown(Gleam::KeyCode::RightAlt)) && (Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftShift) || Gleam::Input::GetButtonDown(Gleam::KeyCode::RightShift)) && Gleam::Input::GetButtonDown(Gleam::KeyCode::O))
#elif defined(PLATFORM_MACOS)
    if ((Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftCommand) || Gleam::Input::GetButtonDown(Gleam::KeyCode::RightCommand)) && (Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftShift) || Gleam::Input::GetButtonDown(Gleam::KeyCode::RightShift)) && Gleam::Input::GetButtonDown(Gleam::KeyCode::O))
#else
    if (false)
#endif
    {
        for (const auto& file : Gleam::IOUtils::OpenFileDialog(Gleam::FileType::Model))
        {
            auto mesh = Gleam::CreateScope<Gleam::StaticMesh>(Gleam::AssetLibrary::ImportModel(file));
            mMeshes.emplace_back(std::move(mesh));
        }
    }
    
    if (!Gleam::Input::GetCursorVisible())
    {
        constexpr float mouseSensitivity = 0.1f;
        mYaw += Gleam::Input::GetAxisX() * mouseSensitivity;
        mPitch += Gleam::Input::GetAxisY() * mouseSensitivity;
        mPitch = Gleam::Math::Clamp(mPitch, -80.0f, 80.0f);
        mCamera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Vector3{ mPitch, mYaw, 0.0f })));
    }

    constexpr float cameraSpeed = 5.0f;
    float deltaTime = static_cast<float>(Gleam::Time::deltaTime);
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::A))
    {
        mCamera.Translate(-mCamera.RightVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::D))
    {
        mCamera.Translate(mCamera.RightVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::W))
    {
        mCamera.Translate(mCamera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::S))
    {
        mCamera.Translate(-mCamera.ForwardVector() * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::Space))
    {
        mCamera.Translate(Gleam::Vector3::up * cameraSpeed * deltaTime);
    }
    if (Gleam::Input::GetButtonDown(Gleam::KeyCode::LeftControl))
    {
        mCamera.Translate(Gleam::Vector3::down * cameraSpeed * deltaTime);
    }
}

void SceneView::OnRender()
{
    mRenderer.UpdateView(mCamera);
    
    constexpr int gridWidth = 32;
    constexpr int gridHeight = 32;
    for (int i = 0; i < gridWidth; i++)
    {
        for (int j = 0; j < gridHeight; j++)
        {
            mRenderer.DrawQuad({float(i - gridWidth / 2), 0.0f, float(j - gridHeight / 2)}, 1.0f, 1.0f, Gleam::Color::HSVToRGB(static_cast<float>(Gleam::Time::time), 1.0f, 1.0f));
        }
    }
    
    for (const auto& mesh : mMeshes)
    {
        auto transform = Gleam::Matrix4::identity;
        for (const auto& submesh : mesh->GetSubmeshDescriptors())
        {
            mRenderer.DrawBoundingBox(submesh.bounds, transform, Gleam::Color32(0, 255, 0, 255));
        }
        mRenderer.DrawMesh(mesh.get(), transform, Gleam::Color32(192, 96, 0, 255));
    }
    
    mRenderer.Render();
}
