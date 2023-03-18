#include "gpch.h"
#include "SceneView.h"
#include "RendererContext.h"

#include "Mesh.h"
#include "Assets/Model.h"
#include "Assets/AssetLibrary.h"

#include "Renderers/DebugRenderer.h"
#include "Renderers/WorldRenderer.h"

using namespace Gleam;

void SceneView::OnCreate()
{
    Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e) -> bool
                                                                      {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            mCursorVisible = !mCursorVisible;
            Gleam::Input::ShowCursor(mCursorVisible);
        }
        return false;
    });
    
    AddRenderer<DebugRenderer>();
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
            const auto& model = AssetLibrary<Model>::Import(file);
            auto mesh = CreateScope<StaticMesh>(model);
        }
    }
}

void SceneView::OnRender()
{
    auto renderer = GetRenderer<DebugRenderer>();
    
    constexpr int gridWidth = 32;
    constexpr int gridHeight = 32;
    for (int i = 0; i < gridWidth; i++)
    {
        for (int j = 0; j < gridHeight; j++)
        {
            renderer->DrawQuad({float(i - gridWidth / 2), 0.0f, float(j - gridHeight / 2)}, 1.0f, 1.0f, Gleam::Color::HSVToRGB(static_cast<float>(Gleam::Time::time), 1.0f, 1.0f));
        }
    }
}
