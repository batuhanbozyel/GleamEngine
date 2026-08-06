//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"
#include "EAssets/EAssetManager.h"
#include "Selection/SelectionSystem.h"
#include "Renderers/InfiniteGridRenderer.h"
#include "Renderers/ViewModeRenderer.h"
#include "Renderers/SelectionOutlineRenderer.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/ImGuiRenderer.h"
#include "Renderer/Renderers/PathTracer.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "View/Widgets/PropertyDrawer.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/WindowSystem.h"

#include "Input/InputSystem.h"

#include "World/World.h"
#include "World/Components/Camera.h"
#include "World/Systems/PickingSystem.h"

#include <imgui.h>

using namespace GEditor;

static void SetEntityWorldTransform(Gleam::Entity& entity, const Gleam::Transform& world)
{
	if (entity.HasParent() == false)
	{
		entity.SetLocalTransform(world);
		return;
	}

	const auto& parent = entity.GetParentEntity().GetWorldTransform();
	const auto invParentRotation = Gleam::Math::Inverse(parent.rotation);
	const float invParentScale = 1.0f / parent.scale;

	entity.SetLocalTransform({
		.position = (invParentRotation * (world.position - parent.position)) * invParentScale,
		.rotation = invParentRotation * world.rotation,
		.scale    = world.scale * invParentScale
	});
}

struct TargetPixel
{
	uint32_t x = 0;
	uint32_t y = 0;
};

// Clamped because a marquee can be dragged past the edges of the viewport image
static TargetPixel ToTargetPixel(const Gleam::Float2& screen, const Gleam::Float2& imageMin, const Gleam::Float2& imageSize, const Gleam::Size& targetSize)
{
	const float u = Gleam::Math::Clamp((screen.x - imageMin.x) / imageSize.x, 0.0f, 1.0f);
	const float v = Gleam::Math::Clamp((screen.y - imageMin.y) / imageSize.y, 0.0f, 1.0f);
	return {
		.x = static_cast<uint32_t>(u * (targetSize.width - 1.0f)),
		.y = static_cast<uint32_t>(v * (targetSize.height - 1.0f))
	};
}

// Rotation and scale come from the active entity, the handles sit at the middle of the selection
static Gleam::Transform GetPivotTransform(Gleam::EntityManager& entityManager, const Gleam::TArray<Gleam::EntityHandle>& targets, Gleam::EntityHandle active)
{
	const auto reference = eastl::find(targets.begin(), targets.end(), active) != targets.end() ? active : targets.front();
	auto pivot = entityManager.GetComponent<Gleam::Entity>(reference).GetWorldTransform();

	if (targets.size() > 1)
	{
		auto center = Gleam::Float3::zero;
		for (auto handle : targets)
		{
			center += entityManager.GetComponent<Gleam::Entity>(handle).GetWorldPosition();
		}
		pivot.position = center / static_cast<float>(targets.size());
	}
	return pivot;
}

// Re-anchors every target against the moved pivot, which reduces to a plain assignment for one entity
static void ApplyPivotDelta(Gleam::EntityManager& entityManager, const Gleam::TArray<Gleam::EntityHandle>& targets, const Gleam::Transform& from, const Gleam::Transform& to)
{
	const auto invFromRotation = Gleam::Math::Inverse(from.rotation);
	const auto deltaRotation = to.rotation * invFromRotation;
	const float scaleRatio = to.scale / from.scale;

	for (auto handle : targets)
	{
		auto& entity = entityManager.GetComponent<Gleam::Entity>(handle);
		auto world = entity.GetWorldTransform();

		const auto offset = invFromRotation * (world.position - from.position);
		world.position = to.position + (to.rotation * (offset * scaleRatio));
		world.rotation = deltaRotation * world.rotation;
		world.scale = world.scale * scaleRatio;

		SetEntityWorldTransform(entity, world);
	}
}

void WorldViewport::OnCreate(Gleam::World* world)
{
	mEditWorld = world;
	mSelection = world->GetSubsystem<SelectionSystem>();

	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	mGridRenderer = new InfiniteGridRenderer();
	mGridRenderer->OnCreate(renderSystem->GetRenderContext());

	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mGridRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mGridRenderer);

	mSelectionOutlineRenderer = new SelectionOutlineRenderer(mSelection);
	mSelectionOutlineRenderer->OnCreate(renderSystem->GetRenderContext());
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mSelectionOutlineRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mSelectionOutlineRenderer);

	mEditWorld->GetEntityManager().ForEach<Gleam::Entity, Gleam::Camera>([&](const Gleam::Entity& entity, const Gleam::Camera& camera)
	{
		if (entity.IsActive())
		{
			mCamera = entity;
		}
	});

	mCameraController = mEditWorld->AddSystem<EditorCameraController>(mCamera);
	
	auto windowSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>();
	Resize(mEditWorld->GetEntityManager(), windowSystem->GetResolution());
}

void WorldViewport::OnDestroy(Gleam::World* world)
{
	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveSharedRenderer(mGridRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->RemoveSharedRenderer(mGridRenderer);
	delete mGridRenderer;

	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveSharedRenderer(mSelectionOutlineRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->RemoveSharedRenderer(mSelectionOutlineRenderer);
	mSelectionOutlineRenderer->OnDestroy(renderSystem->GetRenderContext());
	delete mSelectionOutlineRenderer;
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

		// The image is a drag surface for the gizmo and the selection marquee, so the panel
		// itself only moves from its dock tab
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoMove);

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
		auto drawOperationButton = [this](const char* label, GizmoOperation operation)
		{
			const bool selected = mTransformGizmo.GetOperation() == operation;
			if (selected)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.45f, 0.72f, 1.0f));
			}
			if (ImGui::Button(label))
			{
				mTransformGizmo.SetOperation(operation);
			}
			if (selected)
			{
				ImGui::PopStyleColor();
			}
		};

		ImGui::SetCursorPosX(4.0f);
		ImGui::SetCursorPosY((kToolbarHeight - ImGui::GetFrameHeight()) * 0.5f);
		drawOperationButton("Move", GizmoOperation::Translate);
		ImGui::SameLine(0.0f, 2.0f);
		drawOperationButton("Rotate", GizmoOperation::Rotate);
		ImGui::SameLine(0.0f, 2.0f);
		drawOperationButton("Scale", GizmoOperation::Scale);

		ImGui::SameLine(0.0f, 8.0f);
		const bool localSpace = mTransformGizmo.GetSpace() == GizmoSpace::Local;
		if (ImGui::Button(localSpace ? "Local" : "World"))
		{
			mTransformGizmo.SetSpace(localSpace ? GizmoSpace::World : GizmoSpace::Local);
		}

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
	float displayScale = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>()->GetDisplayScale();
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (mViewportSize != Gleam::Size(viewportSize.x, viewportSize.y))
	{
		mViewportSize.width = viewportSize.x;
		mViewportSize.height = viewportSize.y;
		mViewportSizeChanged = true;
	}

	ImGui::Image(imgui->GetImTextureIDForTexture(passData.sceneTarget), ImVec2(sceneRTsize.width / displayScale, sceneRTsize.height / displayScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

	ImVec2 imageMin = ImGui::GetItemRectMin();
	ImVec2 imageSize = ImGui::GetItemRectSize();
	bool viewportHovered = ImGui::IsItemHovered();

	const Gleam::Float2 rectMin(imageMin.x, imageMin.y);
	const Gleam::Float2 rectSize(imageSize.x, imageSize.y);

	DrawTransformGizmo(rectMin, rectSize);

	bool isFocused = ImGui::IsWindowFocused();
	mCameraController->Enabled = isFocused && mTransformGizmo.IsDragging() == false;

	UpdateSelectionMarquee(rectMin, rectSize, sceneRTsize, viewportHovered);

	auto ctx = ImGui::GetCurrentContext();
	if (isFocused && ctx->IO.MouseClicked[ImGuiMouseButton_Right])
	{
		auto inputSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>();
		mCursorVisible ? inputSystem->HideCursor() : inputSystem->ShowCursor();
		mCursorVisible = !mCursorVisible;
	}
	
	if (isFocused && mCursorVisible)
	{
		if (ImGui::IsKeyPressed(ImGuiKey_W, false))
		{
			mTransformGizmo.SetOperation(GizmoOperation::Translate);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_E, false))
		{
			mTransformGizmo.SetOperation(GizmoOperation::Rotate);
		}
		if (ImGui::IsKeyPressed(ImGuiKey_R, false))
		{
			mTransformGizmo.SetOperation(GizmoOperation::Scale);
		}
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

void WorldViewport::UpdateSelectionMarquee(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize, const Gleam::Size& targetSize, bool viewportHovered)
{
	// Below this the drag is still a click, so a small wobble does not turn into a rectangle
	constexpr float kMarqueeThreshold = 4.0f;

	const auto& io = ImGui::GetIO();
	const Gleam::Float2 mouse(io.MousePos.x, io.MousePos.y);

	if (mMarqueeDragging == false)
	{
		const bool gizmoBusy = mTransformGizmo.IsHovered() || mTransformGizmo.IsDragging();
		if (viewportHovered && mCursorVisible && gizmoBusy == false && io.MouseClicked[ImGuiMouseButton_Left])
		{
			mMarqueeDragging = true;
			mMarqueeMoved = false;
			mMarqueeStart = mouse;
		}
		return;
	}

	if (io.MouseDown[ImGuiMouseButton_Left])
	{
		mMarqueeMoved |= Gleam::Math::Abs(mouse.x - mMarqueeStart.x) > kMarqueeThreshold
					  || Gleam::Math::Abs(mouse.y - mMarqueeStart.y) > kMarqueeThreshold;

		if (mMarqueeMoved)
		{
			const ImVec2 min(Gleam::Math::Min(mMarqueeStart.x, mouse.x), Gleam::Math::Min(mMarqueeStart.y, mouse.y));
			const ImVec2 max(Gleam::Math::Max(mMarqueeStart.x, mouse.x), Gleam::Math::Max(mMarqueeStart.y, mouse.y));

			auto drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(min, max, IM_COL32(66, 115, 184, 64));
			drawList->AddRect(min, max, IM_COL32(120, 170, 235, 255));
		}
		return;
	}

	// A marquee resolves as a region, a plain click stays the single pixel under the cursor
	const auto from = ToTargetPixel(mMarqueeMoved ? mMarqueeStart : mouse, imageMin, imageSize, targetSize);
	const auto to = ToTargetPixel(mouse, imageMin, imageSize, targetSize);

	Gleam::PickingRequest request;
	request.x = Gleam::Math::Min(from.x, to.x);
	request.y = Gleam::Math::Min(from.y, to.y);
	request.width = Gleam::Math::Max(from.x, to.x) - request.x + 1;
	request.height = Gleam::Math::Max(from.y, to.y) - request.y + 1;

	const bool additive = io.KeyCtrl || io.KeySuper;
	mSelection->RequestPick(request, additive ? SelectionMode::Toggle : SelectionMode::Replace);

	mMarqueeDragging = false;
	mMarqueeMoved = false;
}

void WorldViewport::GatherGizmoTargets(Gleam::EntityManager& entityManager)
{
	mGizmoTargets.clear();
	for (auto handle : mSelection->GetSelectedEntities())
	{
		if (entityManager.HasComponent<Gleam::Entity>(handle) == false)
		{
			continue;
		}

		// A child already inherits its parent's delta, applying it again would move it twice
		bool ancestorSelected = false;
		auto parent = entityManager.GetComponent<Gleam::Entity>(handle).GetParent();
		while (parent != Gleam::InvalidEntity)
		{
			if (mSelection->IsSelected(parent))
			{
				ancestorSelected = true;
				break;
			}
			parent = entityManager.GetComponent<Gleam::Entity>(parent).GetParent();
		}

		if (ancestorSelected == false)
		{
			mGizmoTargets.push_back(handle);
		}
	}
}

void WorldViewport::DrawTransformGizmo(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	GatherGizmoTargets(entityManager);
	if (mGizmoTargets.empty())
	{
		return;
	}

	const auto& cameraEntity = entityManager.GetComponent<Gleam::Entity>(mCamera);
	const auto& cameraComponent = entityManager.GetComponent<Gleam::Camera>(mCamera);

	Gleam::Float4x4 view = Gleam::Float4x4::LookTo(cameraEntity.GetWorldPosition(), cameraEntity.ForwardVector(), cameraEntity.UpVector());
	Gleam::Float4x4 projection;
	if (cameraComponent.projectionType == Gleam::ProjectionType::Perspective)
	{
		projection = Gleam::Float4x4::Perspective(Gleam::Math::Deg2Rad(cameraComponent.fov), cameraComponent.aspectRatio, cameraComponent.nearPlane, cameraComponent.farPlane);
	}
	else
	{
		const auto resolution = cameraComponent.GetViewport();
		projection = Gleam::Float4x4::Ortho(resolution.width, resolution.height, cameraComponent.nearPlane, cameraComponent.farPlane);
	}

	GizmoViewport viewport;
	viewport.viewProjection = projection * view;
	viewport.invViewProjection = Gleam::Math::Inverse(viewport.viewProjection);
	viewport.cameraPosition = cameraEntity.GetWorldPosition();
	viewport.projectionScaleY = projection.m[5];
	viewport.rectMin = imageMin;
	viewport.rectSize = imageSize;

	auto pivot = GetPivotTransform(entityManager, mGizmoTargets, mSelection->GetActiveEntity());
	const auto startPivot = pivot;

	const bool inputEnabled = ImGui::IsWindowHovered() && mCursorVisible;
	if (mTransformGizmo.Manipulate(viewport, inputEnabled, pivot))
	{
		ApplyPivotDelta(entityManager, mGizmoTargets, startPivot, pivot);
	}
}

void WorldViewport::Resize(Gleam::EntityManager& entityManager, const Gleam::Size& size)
{
	mViewportSize = size;
	mViewportSizeChanged = false;

	auto windowSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>();
	float displayScale = windowSystem->GetDisplayScale();
	auto& camera = entityManager.GetComponent<Gleam::Camera>(mCamera);
	camera.SetViewport(mViewportSize * displayScale);
}
