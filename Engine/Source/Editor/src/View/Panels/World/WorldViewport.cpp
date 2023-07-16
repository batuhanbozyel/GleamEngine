//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "WorldViewportController.h"

#include "ImGui/ImGuiBackend.h"

#include <imgui.h>

using namespace GEditor;

WorldViewport::WorldViewport()
{
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->AddRenderer<Gleam::DebugRenderer>();
    mViewportSize = GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetResolution();
    
    mEditWorld = Gleam::World::active;
	mController = mEditWorld->AddSystem<WorldViewportController>();
    
    Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e)
    {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            if (mIsFocused)
            {
                auto inputSystem = GameInstance->GetSubsystem<Gleam::InputSystem>();
                mCursorVisible ? inputSystem->HideCursor() : inputSystem->ShowCursor();
                mCursorVisible = !mCursorVisible;
            }
        }
    });
}

void WorldViewport::Update()
{
    Gleam::TextureDescriptor descriptor;
    descriptor.size = mViewportSize;
    mSceneRT = Gleam::CreateRef<Gleam::RenderTexture>(descriptor);
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->SetRenderTarget(mSceneRT);
    
    mController->SetViewportSize(mViewportSize);
    mController->SetViewportFocused(mIsFocused);
}

void WorldViewport::Render()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    mViewportSize.width = viewportSize.x;
    mViewportSize.height = viewportSize.y;
    
    const auto& sceneRTsize = mSceneRT->GetDescriptor().size;
    ImGui::Image(IMGUI_IMAGE_HANDLE(mSceneRT->GetHandle()), ImVec2(sceneRTsize.width, sceneRTsize.height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    mIsFocused = ImGui::IsWindowFocused();
    
    ImGui::End();
    ImGui::PopStyleVar();
}
