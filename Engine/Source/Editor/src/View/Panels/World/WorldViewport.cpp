//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"
#include "EAssets/EAssetManager.h"
#include "Renderer/ImGui/ImGuiBackend.h"
#include "Renderers/InfiniteGridRenderer.h"

#include "Gleam.h"

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
}

void WorldViewport::Update()
{
    if (mViewportSizeChanged)
    {
		Resize(mEditWorld->GetEntityManager(), mViewportSize);
	}
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

		bool isFocused = ImGui::IsWindowFocused();
		mCameraController->Enabled = isFocused;

		auto ctx = ImGui::GetCurrentContext();
		if (isFocused && ctx->IO.MouseClicked[ImGuiMouseButton_Right])
		{
			auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
			mCursorVisible ? inputSystem->HideCursor() : inputSystem->ShowCursor();
			mCursorVisible = !mCursorVisible;
		}

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("EDITOR_ASSET"))
			{
				IM_ASSERT(payload->DataSize == sizeof(AssetItem));
				const auto& assetItem = *(const AssetItem*)payload->Data;

				if (assetItem.type == Gleam::Reflection::GetClass<Gleam::Prefab>().Guid())
				{
					auto& entity = mEditWorld->GetEntityManager().CreateFromPrefab(assetItem.reference);
				}
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
}
