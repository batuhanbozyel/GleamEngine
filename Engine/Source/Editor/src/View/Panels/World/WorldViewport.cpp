//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"

#include "Renderer/ImGui/ImGuiBackend.h"
#include "Renderers/InfiniteGridRenderer.h"

using namespace GEditor;

WorldViewport::WorldViewport(Gleam::World* world)
	: mEditWorld(world)
{
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->AddRenderer<InfiniteGridRenderer>();
    mViewportSize = GameInstance->GetSubsystem<Gleam::WindowSystem>()->GetResolution();
    
	mController = mEditWorld->AddSystem<EditorCameraController>();
    mController->Resize(mEditWorld->GetEntityManager(), mViewportSize);
    
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
	mController->Enabled = mIsFocused;
    if (mViewportSizeChanged)
    {
		mViewportSizeChanged = false;
        mController->Resize(mEditWorld->GetEntityManager(), mViewportSize);
        Gleam::EventDispatcher<Gleam::RendererResizeEvent>::Publish(Gleam::RendererResizeEvent(mViewportSize));
    }
    
    Gleam::TextureDescriptor descriptor;
    descriptor.size = mViewportSize;
    descriptor.usage = Gleam::TextureUsage_Attachment | Gleam::TextureUsage_Sampled;
    GameInstance->GetSubsystem<Gleam::RenderSystem>()->SetRenderTarget(descriptor);
}

void WorldViewport::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		const auto& sceneRTsize = passData.sceneTarget.GetTexture().GetDescriptor().size;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		if (mViewportSize != Gleam::Size(viewportSize.x, viewportSize.y))
		{
			mViewportSize.width = viewportSize.x;
			mViewportSize.height = viewportSize.y;
			mViewportSizeChanged = true;
		}
		
		ImGui::Image(Gleam::ImGuiBackend::GetImTextureIDForTexture(passData.sceneTarget), ImVec2(sceneRTsize.width, sceneRTsize.height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
		mIsFocused = ImGui::IsWindowFocused();

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
			{
				IM_ASSERT(payload->DataSize == sizeof(Gleam::AssetReference));
				const auto& asset = *(const Gleam::AssetReference*)payload->Data;
				if (asset.GetType() == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
				{

				}
			}
			ImGui::EndDragDropTarget();
		}
		
		ImGui::End();
		ImGui::PopStyleVar();
	});
}
