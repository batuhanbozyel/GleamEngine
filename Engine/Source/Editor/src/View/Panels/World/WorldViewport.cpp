//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"
#include "EAssets/AssetRegistry.h"
#include "Renderer/ImGui/ImGuiBackend.h"
#include "Renderers/InfiniteGridRenderer.h"

using namespace GEditor;

void WorldViewport::Init(Gleam::World* world)
{
	mEditWorld = world;
    Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>()->AddRenderer<InfiniteGridRenderer>();
    mViewportSize = Gleam::Globals::Engine->GetResolution();

	mEditWorld->GetEntityManager().ForEach<Gleam::Entity, Gleam::Camera>([&](const Gleam::Entity& entity, const Gleam::Camera& camera)
	{
		if (entity.IsActive())
		{
			mCamera = entity;
		}
	});

	mCameraController = mEditWorld->AddSystem<EditorCameraController>(mCamera);
    Resize(mEditWorld->GetEntityManager(), mViewportSize);
    
    Gleam::EventDispatcher<Gleam::MouseButtonPressedEvent>::Subscribe([&](Gleam::MouseButtonPressedEvent e)
    {
        if (e.GetMouseButton() == Gleam::MouseButton::Right)
        {
            if (mIsFocused)
            {
                auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
                mCursorVisible ? inputSystem->HideCursor() : inputSystem->ShowCursor();
                mCursorVisible = !mCursorVisible;
            }
        }
    });
}

void WorldViewport::Update()
{
	mCameraController->Enabled = mIsFocused;
    if (mViewportSizeChanged)
    {
		Resize(mEditWorld->GetEntityManager(), mViewportSize);
    }
    
    Gleam::TextureDescriptor descriptor;
    descriptor.name = "Editor::Backbuffer";
    descriptor.size = mViewportSize;
    descriptor.usage = Gleam::TextureUsage_Attachment | Gleam::TextureUsage_Sampled;
    Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>()->SetBackbuffer(descriptor);
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
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EDITOR_ASSET"))
			{
				IM_ASSERT(payload->DataSize == sizeof(AssetItem));
				const auto& assetItem = *(const AssetItem*)payload->Data;

				// TODO: remove this, it is for testing only
				/*if (assetItem.type == Gleam::Reflection::GetClass<Gleam::MeshDescriptor>().Guid())
				{
					auto materialSystem = Gleam::Globals::GameInstance->GetSubsystem<Gleam::MaterialSystem>();
					auto assetManager = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>();
					auto mesh = assetManager->Get<Gleam::MeshDescriptor>(assetItem.reference);

					Gleam::AssetReference meshRef = assetItem.reference;
					Gleam::TArray<Gleam::AssetReference> materialRefs;
					for (uint32_t i = 0; i < mesh.submeshes.size(); ++i)
					{
						Gleam::AssetReference material = { .guid = Gleam::Guid("044B0097-7F40-438D-9FD4-3606E73EDFD6") };
						materialRefs.push_back(material);
					}

					auto& entityManager = mEditWorld->GetEntityManager();
					auto& entity = entityManager.CreateEntity();
					entity.AddComponent<Gleam::MeshRenderer>(meshRef, materialRefs);
				}*/
			}
			ImGui::EndDragDropTarget();
		}
		
		ImGui::End();
		ImGui::PopStyleVar();
	});
}

void WorldViewport::Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size)
{
	mViewportSize = size;
	mViewportSizeChanged = false;

	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCamera);
	camera.SetViewport(mViewportSize);

	Gleam::EventDispatcher<Gleam::RendererResizeEvent>::Publish(Gleam::RendererResizeEvent(mViewportSize));
}
