//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "WorldViewportController.h"

#include "ImGui/ImGuiBackend.h"
#include "Renderers/InfiniteGridRenderer.h"

using namespace GEditor;

WorldViewport::WorldViewport()
{
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->AddRenderer<InfiniteGridRenderer>();
    mViewportSize = GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetResolution();
    
    mEditWorld = Gleam::World::active;
	mController = mEditWorld->AddSystem<WorldViewportController>();
    mController->SetViewportSize(mViewportSize);
    
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
    if (mViewportSize != mController->GetViewportSize())
    {
        mController->SetViewportSize(mViewportSize);
        Gleam::EventDispatcher<Gleam::RendererResizeEvent>::Publish(Gleam::RendererResizeEvent(mViewportSize));
    }
    
    Gleam::TextureDescriptor descriptor;
    descriptor.size = mViewportSize;
    descriptor.usage = Gleam::TextureUsage_Attachment | Gleam::TextureUsage_Sampled;
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->SetRenderTarget(descriptor);
    
    mController->SetViewportFocused(mIsFocused);
}

void WorldViewport::Render()
{
    const auto& sceneRT = GameInstance->GetSubsystem<Gleam::RenderSystem>()->GetRenderTarget();
    const auto& sceneRTsize = sceneRT.GetDescriptor().size;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    mViewportSize.width = viewportSize.x;
    mViewportSize.height = viewportSize.y;
    
    ImGui::Image(ImGuiBackend::GetImTextureIDForTexture(sceneRT), ImVec2(sceneRTsize.width, sceneRTsize.height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
    mIsFocused = ImGui::IsWindowFocused();
    
    ImGui::End();
    ImGui::PopStyleVar();
}
