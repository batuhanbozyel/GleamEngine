//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"
#include "EAssets/EAssetManager.h"
#include "Renderers/InfiniteGridRenderer.h"
#include "Renderers/ViewModeRenderer.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/ImGuiRenderer.h"
#include "Renderer/Renderers/PathTracer.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "View/Widgets/PropertyDrawer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"

#include "Input/InputSystem.h"

#include "World/World.h"
#include "World/Components/Camera.h"

using namespace GEditor;

void WorldViewport::OnCreate(Gleam::World* world)
{
	mEditWorld = world;
    mViewportSize = Gleam::Globals::Engine->GetResolution();
	
	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	mGridRenderer = new InfiniteGridRenderer();
	mGridRenderer->OnCreate(renderSystem->GetRenderContext());

	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mGridRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mGridRenderer);

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

void WorldViewport::OnDestroy(Gleam::World* world)
{
	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveSharedRenderer(mGridRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->RemoveSharedRenderer(mGridRenderer);
	delete mGridRenderer;
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
	imgui->PushView([=, this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		DrawToolbar();
		DrawViewport(imgui, passData);
		
		ImGui::End();
		ImGui::PopStyleVar();
	});
}

void WorldViewport::DrawToolbar()
{
	constexpr float kToolbarHeight = 26.0f;
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.145f, 0.145f, 0.145f, 1.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::BeginChild("##ViewportToolbar", ImVec2(ImGui::GetContentRegionAvail().x, kToolbarHeight),
		ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
	{
		constexpr const char* kRenderSettingsLabel = "Render Settings";
		constexpr float kPopupWidth = 240.0f;
		float labelWidth = ImGui::CalcTextSize(kRenderSettingsLabel).x + ImGui::GetStyle().FramePadding.x * 2.0f;
		float arrowWidth = ImGui::GetFrameHeight();
		float buttonWidth = labelWidth + arrowWidth;
		ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonWidth - 4.0f);
		ImGui::SetCursorPosY((kToolbarHeight - ImGui::GetFrameHeight()) * 0.5f);

		if (ImGui::Button(kRenderSettingsLabel))
		{
			ImGui::OpenPopup("##RenderSettingsPopup");
		}
		ImGui::SameLine(0.0f, 0.0f);
		if (ImGui::ArrowButton("##RenderSettingsArrow", ImGuiDir_Down))
		{
			ImGui::OpenPopup("##RenderSettingsPopup");
		}

		ImVec2 buttonMax = ImGui::GetItemRectMax();
		ImGui::SetNextWindowPos(ImVec2(buttonMax.x - kPopupWidth, buttonMax.y), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(kPopupWidth, 0.0f), ImGuiCond_Always);
		if (ImGui::BeginPopup("##RenderSettingsPopup"))
		{
			auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
			auto activePath = renderSystem->GetRenderPath();
			const auto& enumDesc = Gleam::Reflection::GetEnum<Gleam::RenderPath>();

			auto prevPath = activePath;
			PropertyDrawer::DrawEnumOptions("Render Path", enumDesc, &activePath, 80.0f);
			if (activePath != prevPath)
			{
				renderSystem->SetRenderPath(activePath);
			}

			if (activePath == Gleam::RenderPath::Default)
			{
				auto viewModeRenderer = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->GetRenderer<ViewModeRenderer>();
				auto activeViewMode = viewModeRenderer->GetViewMode();
				const auto& viewModeEnumDesc = Gleam::Reflection::GetEnum<Gleam::ViewMode>();
				
				auto prevViewMode = activeViewMode;
				PropertyDrawer::DrawEnumOptions("View Mode", viewModeEnumDesc, &activeViewMode, 80.0f);
				if (activeViewMode != prevViewMode)
				{
					viewModeRenderer->SetViewMode(activeViewMode);
				}
			}
			else if (activePath == Gleam::RenderPath::PathTracing)
			{
				auto pathTracer = renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->GetRenderer<Gleam::PathTracer>();
				auto settings = pathTracer->GetSettings();
				PropertyDrawer::DrawClassFields(&settings, Gleam::Reflection::GetClass<Gleam::PathTracerSettings>());
				pathTracer->SetSettings(settings);
			}
			ImGui::EndPopup();
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
}

void WorldViewport::DrawViewport(Gleam::ImGuiRenderer* imgui, const Gleam::ImGuiPassData& passData)
{
	const auto& sceneRTsize = passData.sceneTarget.GetTexture().GetDescriptor().size;
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (mViewportSize != Gleam::Size(viewportSize.x, viewportSize.y))
	{
		mViewportSize.width = viewportSize.x;
		mViewportSize.height = viewportSize.y;
		mViewportSizeChanged = true;
	}

	ImGui::Image(imgui->GetImTextureIDForTexture(passData.sceneTarget), ImVec2(sceneRTsize.width, sceneRTsize.height), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

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
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("GLEAM_PREFAB"))
		{
			IM_ASSERT(payload->DataSize == sizeof(AssetItem));
			const auto& assetItem = *(const AssetItem*)payload->Data;
			auto& entity = mEditWorld->GetEntityManager().CreateFromPrefab(assetItem.reference);
		}
		ImGui::EndDragDropTarget();
	}
}

void WorldViewport::Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size)
{
	mViewportSize = size;
	mViewportSizeChanged = false;

	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCamera);
	camera.SetViewport(mViewportSize);
}
