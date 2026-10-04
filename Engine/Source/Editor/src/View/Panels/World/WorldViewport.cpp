//
//  WorldViewport.cpp
//  Editor
//
//  Created by Batuhan Bozyel on 26.03.2023.
//

#include "WorldViewport.h"
#include "EditorCameraController.h"
#include "EditorCamera.h"
#include "Config/EditorConfigSystem.h"
#include "EAssets/EAssetManager.h"
#include "EWorld/EWorldManager.h"
#include "Selection/SelectionSystem.h"
#include "Undo/UndoSystem.h"
#include "Renderers/InfiniteGridRenderer.h"
#include "Renderers/ViewModeRenderer.h"

#include <Editor.Reflection.generated.h>
#include "Renderers/SelectionOutlineRenderer.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/DebugRenderer.h"
#include "Renderer/Renderers/LineRenderer.h"
#include "Renderer/Renderers/ImGuiRenderer.h"
#include "Renderer/Renderers/PathTracer.h"
#include "Renderer/Renderers/PostProcessStack.h"
#include "View/Widgets/PropertyDrawer.h"
#include "View/Widgets/EditorWidgets.h"
#include "View/ViewStack.h"
#include "View/GleamTheme.h"
#include "View/IconsLucide.h"

#include "Core/Globals.h"
#include "Core/Engine.h"
#include "Core/Application.h"
#include "Core/WindowSystem.h"

#include "Input/InputSystem.h"

#include "World/World.h"
#include "World/Components/Camera.h"
#include "World/Systems/PickingSystem.h"

#include <imgui.h>

#include <cstdio>

using namespace GEditor;

#if defined(USE_DIRECTX_RENDERER)
static constexpr const char* kBackendName = "D3D12";
#elif defined(USE_METAL_RENDERER)
static constexpr const char* kBackendName = "Metal";
#endif

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

static TargetPixel ToTargetPixel(const Gleam::Float2& screen, const Gleam::Float2& imageMin, const Gleam::Float2& imageSize, const Gleam::Size& targetSize)
{
	const float u = Gleam::Math::Clamp((screen.x - imageMin.x) / imageSize.x, 0.0f, 1.0f);
	const float v = Gleam::Math::Clamp((screen.y - imageMin.y) / imageSize.y, 0.0f, 1.0f);
	return {
		.x = static_cast<uint32_t>(u * (targetSize.width - 1.0f)),
		.y = static_cast<uint32_t>(v * (targetSize.height - 1.0f))
	};
}

static Gleam::Transform GetPivotTransform(const Gleam::EntityManager& entityManager, const Gleam::TArray<Gleam::EntityHandle>& targets, Gleam::EntityHandle active)
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

WorldViewport::WorldViewport(Gleam::World* world)
	: mEditWorld(world)
{

}

void WorldViewport::OnCreate(Gleam::Application* app)
{
	mFonts = &app->GetSubsystem<ViewStack>()->GetFonts();
	mWorldManager = app->GetSubsystem<EWorldManager>();
	mSelectionSystem = mEditWorld->GetSubsystem<SelectionSystem>();
	mUndoSystem = mEditWorld->GetSubsystem<UndoSystem>();

	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	mGridRenderer = new InfiniteGridRenderer();
	mGridRenderer->OnCreate(renderSystem->GetRenderContext());

	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mGridRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mGridRenderer);
	auto lineRenderer = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddRenderer<Gleam::LineRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddRenderer<Gleam::DebugRenderer>(lineRenderer);

	mSelectionOutlineRenderer = new SelectionOutlineRenderer(mSelectionSystem);
	mSelectionOutlineRenderer->OnCreate(renderSystem->GetRenderContext());
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->AddSharedRenderer(mSelectionOutlineRenderer);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->AddSharedRenderer(mSelectionOutlineRenderer);

	auto windowSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>();
	auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
	const auto& cameraState = editorConfig->Register<Gleam::EditorCameraState>();

	auto& entityManager = mEditWorld->GetEntityManager();
	auto& camera = entityManager.CreateEntity("Editor Camera", Gleam::Guid::NewGuid());
	camera.SetTranslation(cameraState.position);
	camera.SetRotation(Gleam::Quaternion(Gleam::Math::Deg2Rad(Gleam::Float3{ cameraState.pitch, cameraState.yaw, 0.0f })));
	entityManager.AddComponent<Gleam::Camera>(camera, windowSystem->GetResolution(), Gleam::ProjectionType::Perspective);
	auto& editorCamera = entityManager.AddComponent<Gleam::EditorCamera>(camera);
	editorCamera.yaw = cameraState.yaw;
	editorCamera.pitch = cameraState.pitch;
	mCamera = camera;

	mCameraController = mEditWorld->AddSystem<EditorCameraController>(mCamera);
	mPhysicsVisualization = mEditWorld->AddSystem<PhysicsVisualizationSystem>();
	Resize(entityManager, windowSystem->GetResolution());
}

void WorldViewport::OnDestroy(Gleam::Application* app)
{
	auto& entityManager = mEditWorld->GetEntityManager();
	const auto& camera = entityManager.GetComponent<Gleam::Entity>(mCamera);
	const auto& editorCamera = entityManager.GetComponent<Gleam::EditorCamera>(mCamera);
	auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
	editorConfig->Set(Gleam::EditorCameraState{
		.position = camera.GetLocalPosition(),
		.yaw = editorCamera.yaw,
		.pitch = editorCamera.pitch
	});

	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	renderSystem->SetCameraOverride(Gleam::InvalidEntity);
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveRenderer<ViewModeRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveRenderer<Gleam::DebugRenderer>();
	renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->RemoveRenderer<Gleam::LineRenderer>();
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
	auto playWorld = mWorldManager->GetPlayWorld();
	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	renderSystem->SetCameraOverride(playWorld ? Gleam::InvalidEntity : mCamera);

	if (playWorld and mCursorVisible == false)
	{
		Gleam::Globals::Engine->GetSubsystem<Gleam::InputSystem>()->ShowCursor();
		mCursorVisible = true;
	}

    if (mViewportSizeChanged or mViewportWorld != playWorld)
    {
		Resize(mEditWorld->GetEntityManager(), mViewportSize);
	}
}

void WorldViewport::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([=, this](const Gleam::ImGuiPassData& passData)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, Gleam::Theme::ViewportBg);
		const bool visible = ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		if (visible)
		{
			const ImVec2 cursor = ImGui::GetCursorScreenPos();
			const ImVec2 available = ImGui::GetContentRegionAvail();
			const Gleam::Float2 contentMin(cursor.x, cursor.y);
			const Gleam::Float2 contentSize(available.x, available.y);

			DrawViewport(imgui, passData);
			DrawToolbar(contentMin, contentSize);
			DrawStats(contentMin, contentSize, passData);
		}
		ImGui::End();
	});
}

void WorldViewport::DrawToolbar(const Gleam::Float2& contentMin, const Gleam::Float2& contentSize)
{
	constexpr float kBandHeight = 48.0f;
	constexpr float kEdge = 10.0f;
	constexpr float kButtonHeight = 30.0f;
	constexpr float kSquareButton = 32.0f;
	constexpr float kPopupGap = 4.0f;

	auto drawList = ImGui::GetWindowDrawList();
	const float buttonY = contentMin.y + (kBandHeight - kButtonHeight) * 0.5f;
	const float centerY = buttonY + kButtonHeight * 0.5f;
	bool hovered = false;

	// Gizmo operation
	ImGui::SetCursorScreenPos(ImVec2(contentMin.x + kEdge, buttonY));
	const ImVec2 modesMin = ImGui::GetCursorScreenPos();
	DrawGizmoModes();
	hovered |= ImGui::IsMouseHoveringRect(modesMin, ImVec2(modesMin.x + 94.0f, modesMin.y + kButtonHeight));
	float x = modesMin.x + 94.0f + 10.0f;

	drawList->AddLine(ImVec2(x, centerY - 10.0f), ImVec2(x, centerY + 10.0f), Widgets::ColorU32(Gleam::Theme::Text, 0.18f));
	x += 11.0f;

	// Transform space
	const bool localSpace = mTransformGizmo.GetSpace() == GizmoSpace::Local;
	ImGui::SetCursorScreenPos(ImVec2(x, buttonY));
	if (Widgets::OverlayButton("##Space", localSpace ? ICON_LC_AXIS_3D : ICON_LC_GLOBE, ImVec2(kSquareButton, kButtonHeight)))
	{
		mTransformGizmo.SetSpace(localSpace ? GizmoSpace::World : GizmoSpace::Local);
	}
	hovered |= ImGui::IsItemHovered();
	ImGui::SetItemTooltip(localSpace ? "Transform space: Local" : "Transform space: World");
	x += kSquareButton + 10.0f;

	// Snap
	{
		const float step = mTransformGizmo.GetSnapStep();
		char snapText[16] = {};
		if (mTransformGizmo.GetOperation() == GizmoOperation::Rotate)
		{
			std::snprintf(snapText, sizeof(snapText), "%.0f\xC2\xB0", step);
		}
		else
		{
			std::snprintf(snapText, sizeof(snapText), "%.2f", step);
		}

		const float iconWidth = ImGui::CalcTextSize(ICON_LC_MAGNET).x;
		ImGui::PushFont(mFonts->mono);
		const ImVec2 textSize = ImGui::CalcTextSize(snapText);
		ImGui::PopFont();

		const ImVec2 min(x, buttonY);
		const ImVec2 max(x + 8.0f + iconWidth + 6.0f + textSize.x + 8.0f, buttonY + kButtonHeight);
		ImGui::SetCursorScreenPos(min);
		if (ImGui::InvisibleButton("##Snap", ImVec2(max.x - min.x, kButtonHeight)))
		{
			mTransformGizmo.SetSnapping(mTransformGizmo.IsSnapping() == false);
		}
		const bool snapHovered = ImGui::IsItemHovered();
		hovered |= snapHovered;
		ImGui::SetItemTooltip("Snap (hold Ctrl to invert)");

		drawList->AddRectFilled(min, max, snapHovered ? Widgets::ColorU32(Gleam::Theme::Control, 0.9f) : Widgets::ColorU32(Gleam::Theme::Background, 0.72f), 6.0f);
		drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::BorderStrong), 6.0f);
		Widgets::DrawIcon(drawList, ICON_LC_MAGNET, ImVec2(min.x + 8.0f + iconWidth * 0.5f, centerY), mTransformGizmo.IsSnapping() ? Gleam::Theme::Accent : Gleam::Theme::TextSecondary);

		ImGui::PushFont(mFonts->mono);
		drawList->AddText(ImVec2(min.x + 8.0f + iconWidth + 6.0f, centerY - ImGui::GetFontSize() * 0.5f), Widgets::ColorU32(Gleam::Theme::TextMuted), snapText);
		ImGui::PopFont();
	}

	DrawPlayControls(contentMin.x + contentSize.x * 0.5f, buttonY, kButtonHeight, hovered);

	// Render settings
	float right = contentMin.x + contentSize.x - kEdge;
	{
		const float slidersWidth = ImGui::CalcTextSize(ICON_LC_SLIDERS_HORIZONTAL).x;
		ImGui::PushFont(mFonts->footnote);
		const float chevronWidth = ImGui::CalcTextSize(ICON_LC_CHEVRON_DOWN).x;
		ImGui::PopFont();

		const float width = 8.0f + slidersWidth + 4.0f + chevronWidth + 8.0f;
		const ImVec2 min(right - width, buttonY);
		const ImVec2 max(right, buttonY + kButtonHeight);
		ImGui::SetCursorScreenPos(min);
		if (ImGui::InvisibleButton("##RenderSettings", ImVec2(width, kButtonHeight)))
		{
			ImGui::OpenPopup("##RenderSettingsPopup");
		}
		const bool settingsHovered = ImGui::IsItemHovered();
		hovered |= settingsHovered;
		ImGui::SetItemTooltip("Render settings");

		drawList->AddRectFilled(min, max, settingsHovered ? Widgets::ColorU32(Gleam::Theme::ControlHover, 0.9f) : Widgets::ColorU32(Gleam::Theme::Control, 0.85f), 6.0f);
		drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::BorderStrong), 6.0f);
		Widgets::DrawIcon(drawList, ICON_LC_SLIDERS_HORIZONTAL, ImVec2(min.x + 8.0f + slidersWidth * 0.5f, centerY), Gleam::Theme::Text);
		ImGui::PushFont(mFonts->footnote);
		Widgets::DrawIcon(drawList, ICON_LC_CHEVRON_DOWN, ImVec2(max.x - 8.0f - chevronWidth * 0.5f, centerY), Gleam::Theme::Text);
		ImGui::PopFont();

		ImGui::SetNextWindowPos(ImVec2(max.x, max.y + kPopupGap), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(280.0f, 0.0f), ImGuiCond_Always);
		DrawRenderSettingsPopup();
		right = min.x - 8.0f;
	}

	// View mode
	ImGui::SetCursorScreenPos(ImVec2(right - kSquareButton, buttonY));
	if (Widgets::OverlayButton("##ViewMode", ICON_LC_SUN, ImVec2(kSquareButton, kButtonHeight)))
	{
		ImGui::OpenPopup("##ViewModePopup");
	}
	hovered |= ImGui::IsItemHovered();
	ImGui::SetItemTooltip("View mode");

	ImGui::SetNextWindowPos(ImVec2(right, buttonY + kButtonHeight + kPopupGap), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_Always);
	if (ImGui::BeginPopup("##ViewModePopup"))
	{
		auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
		if (renderSystem->GetRenderPath() == Gleam::RenderPath::Default)
		{
			auto viewModeRenderer = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->GetRenderer<ViewModeRenderer>();
			auto activeViewMode = viewModeRenderer->GetViewMode();
			const auto previousViewMode = activeViewMode;
			PropertyDrawer::DrawEnumOptions("View Mode", Gleam::Reflection::GetEnum<ViewMode>(), &activeViewMode, 96.0f);
			if (activeViewMode != previousViewMode)
			{
				viewModeRenderer->SetViewMode(activeViewMode);
			}
		}
		else
		{
			ImGui::TextDisabled("View modes need the default render path");
		}
		ImGui::EndPopup();
	}
	right -= kSquareButton + 8.0f;

	// Camera
	ImGui::SetCursorScreenPos(ImVec2(right - kSquareButton, buttonY));
	if (Widgets::OverlayButton("##Camera", ICON_LC_BOX, ImVec2(kSquareButton, kButtonHeight)))
	{
		ImGui::OpenPopup("##CameraPopup");
	}
	hovered |= ImGui::IsItemHovered();
	ImGui::SetItemTooltip("Camera");

	ImGui::SetNextWindowPos(ImVec2(right, buttonY + kButtonHeight + kPopupGap), ImGuiCond_Always, ImVec2(1.0f, 0.0f));
	ImGui::SetNextWindowSize(ImVec2(260.0f, 0.0f), ImGuiCond_Always);
	DrawCameraPopup();

	mToolbarHovered = hovered;
}

void WorldViewport::DrawGizmoModes()
{
	constexpr float kPadding = 2.0f;
	constexpr ImVec2 kButtonSize(30.0f, 26.0f);

	struct Mode
	{
		const char* id;
		const char* icon;
		const char* tooltip;
		GizmoOperation operation;
	};
	constexpr Mode kModes[] = {
		{ "##Move", ICON_LC_MOVE, "Move (W)", GizmoOperation::Translate },
		{ "##Rotate", ICON_LC_ROTATE_CW, "Rotate (E)", GizmoOperation::Rotate },
		{ "##Scale", ICON_LC_SCALING, "Scale (R)", GizmoOperation::Scale }
	};

	auto drawList = ImGui::GetWindowDrawList();
	const ImVec2 groupMin = ImGui::GetCursorScreenPos();
	const ImVec2 groupMax(groupMin.x + kPadding * 2.0f + kButtonSize.x * 3.0f, groupMin.y + kPadding * 2.0f + kButtonSize.y);
	drawList->AddRectFilled(groupMin, groupMax, Widgets::ColorU32(Gleam::Theme::Background, 0.72f), 6.0f);
	drawList->AddRect(groupMin, groupMax, Widgets::ColorU32(Gleam::Theme::Border), 6.0f);

	for (uint32_t i = 0; i < 3; ++i)
	{
		const auto& mode = kModes[i];
		const ImVec2 min(groupMin.x + kPadding + kButtonSize.x * i, groupMin.y + kPadding);
		const ImVec2 max(min.x + kButtonSize.x, min.y + kButtonSize.y);

		ImGui::SetCursorScreenPos(min);
		if (ImGui::InvisibleButton(mode.id, kButtonSize))
		{
			mTransformGizmo.SetOperation(mode.operation);
		}
		const bool hovered = ImGui::IsItemHovered();
		ImGui::SetItemTooltip("%s", mode.tooltip);

		const bool active = mTransformGizmo.GetOperation() == mode.operation;
		if (active)
		{
			drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Accent), 4.0f);
		}
		const auto& iconColor = active ? Gleam::Theme::OnAccent : (hovered ? Gleam::Theme::Text : Gleam::Theme::TextSecondary);
		Widgets::DrawIcon(drawList, mode.icon, ImVec2((min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f), iconColor);
	}
}

void WorldViewport::DrawPlayControls(float centerX, float buttonY, float buttonHeight, bool& hovered)
{
	constexpr float kButtonWidth = 32.0f;
	constexpr float kSpacing = 4.0f;

	const auto playState = mWorldManager->GetPlayState();
	const bool simulating = mWorldManager->IsSimulating();
	const bool playing = playState == PlayState::Playing;

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, ImGuiInputFlags_RouteGlobal))
	{
		if (simulating)
		{
			mWorldManager->RequestStop();
		}
		else
		{
			mWorldManager->RequestPlay();
		}
	}
	if (simulating and ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_P, ImGuiInputFlags_RouteGlobal))
	{
		if (playing)
		{
			mWorldManager->RequestPause();
		}
		else
		{
			mWorldManager->RequestPlay();
		}
	}
	if (playState == PlayState::Paused and ImGui::Shortcut(ImGuiKey_F10, ImGuiInputFlags_RouteGlobal))
	{
		mWorldManager->RequestStep();
	}

	float x = centerX - (kButtonWidth * 3.0f + kSpacing * 2.0f) * 0.5f;

	ImGui::SetCursorScreenPos(ImVec2(x, buttonY));
	if (Widgets::OverlayButton("##PlayPause", playing ? ICON_LC_PAUSE : ICON_LC_PLAY, ImVec2(kButtonWidth, buttonHeight), playing ? Gleam::Theme::Warning : Gleam::Theme::Success))
	{
		if (playing)
		{
			mWorldManager->RequestPause();
		}
		else
		{
			mWorldManager->RequestPlay();
		}
	}
	hovered |= ImGui::IsItemHovered();
	ImGui::SetItemTooltip(playing ? "Pause (Ctrl+Shift+P)" : (simulating ? "Resume (Ctrl+Shift+P)" : "Play (Ctrl+P)"));
	x += kButtonWidth + kSpacing;

	ImGui::BeginDisabled(playState != PlayState::Paused);
	ImGui::SetCursorScreenPos(ImVec2(x, buttonY));
	if (Widgets::OverlayButton("##Step", ICON_LC_STEP_FORWARD, ImVec2(kButtonWidth, buttonHeight), Gleam::Theme::Info))
	{
		mWorldManager->RequestStep();
	}
	hovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	ImGui::SetItemTooltip("Step (F10)");
	ImGui::EndDisabled();
	x += kButtonWidth + kSpacing;

	ImGui::BeginDisabled(simulating == false);
	ImGui::SetCursorScreenPos(ImVec2(x, buttonY));
	if (Widgets::OverlayButton("##Stop", ICON_LC_SQUARE, ImVec2(kButtonWidth, buttonHeight), Gleam::Theme::Error))
	{
		mWorldManager->RequestStop();
	}
	hovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled);
	ImGui::SetItemTooltip("Stop (Ctrl+P)");
	ImGui::EndDisabled();
}

void WorldViewport::DrawCameraPopup()
{
	if (ImGui::BeginPopup("##CameraPopup") == false)
	{
		return;
	}

	auto& camera = mEditWorld->GetEntityManager().GetComponent<Gleam::Camera>(mCamera);
	PropertyDrawer::DrawEnumOptions("Projection", Gleam::Reflection::GetEnum<Gleam::ProjectionType>(), &camera.projectionType, 96.0f);
	if (camera.projectionType == Gleam::ProjectionType::Perspective)
	{
		PropertyDrawer::DrawScalarControl("Field of view", camera.fov, 60.0f, 96.0f);
	}
	else
	{
		PropertyDrawer::DrawScalarControl("Size", camera.orthographicSize, 5.0f, 96.0f);
	}
	PropertyDrawer::DrawScalarControl("Near plane", camera.nearPlane, 0.1f, 96.0f);
	PropertyDrawer::DrawScalarControl("Far plane", camera.farPlane, 1000.0f, 96.0f);
	ImGui::EndPopup();
}

void WorldViewport::DrawRenderSettingsPopup()
{
	if (ImGui::BeginPopup("##RenderSettingsPopup") == false)
	{
		return;
	}

	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	auto activePath = renderSystem->GetRenderPath();
	const auto& enumDesc = Gleam::Reflection::GetEnum<Gleam::RenderPath>();

	auto prevPath = activePath;
	PropertyDrawer::DrawEnumOptions("Render Path", enumDesc, &activePath, 96.0f);
	if (activePath != prevPath)
	{
		renderSystem->SetRenderPath(activePath);
	}

	if (activePath == Gleam::RenderPath::PathTracing)
	{
		auto pathTracer = renderSystem->GetRenderPipeline(Gleam::RenderPath::PathTracing)->GetRenderer<Gleam::PathTracer>();
		auto settings = pathTracer->GetSettings();
		PropertyDrawer::DrawClassFields(&settings, Gleam::Reflection::GetClass<Gleam::PathTracerSettings>(), 96.0f);
		pathTracer->SetSettings(settings);
	}

	auto physicsSettings = mPhysicsVisualization->GetSettings();
	PropertyDrawer::DrawClass("Physics", &physicsSettings, Gleam::Reflection::GetClass<Gleam::PhysicsVisualizationSettings>(), 96.0f);
	mPhysicsVisualization->SetSettings(physicsSettings);

	auto debugRenderer = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->GetRenderer<Gleam::DebugRenderer>();
	auto debugSettings = debugRenderer->GetSettings();
	PropertyDrawer::DrawClass("Debug Lines", &debugSettings, Gleam::Reflection::GetClass<Gleam::DebugRendererSettings>(), 96.0f);
	debugRenderer->SetSettings(debugSettings);

	ImGui::EndPopup();
}

void WorldViewport::DrawStats(const Gleam::Float2& contentMin, const Gleam::Float2& contentSize, const Gleam::ImGuiPassData& passData)
{
	constexpr float kMargin = 12.0f;
	constexpr ImVec2 kPadding(10.0f, 6.0f);
	constexpr float kLineGap = 2.0f;

	auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	const char* pathName = renderSystem->GetRenderPath() == Gleam::RenderPath::PathTracing ? "Path tracing" : "Visibility buffer";

	char pipelineText[64] = {};
	std::snprintf(pipelineText, sizeof(pipelineText), "%s \xC2\xB7 %s", kBackendName, pathName);

	char frameText[64] = {};
	const float frameTime = ImGui::GetIO().DeltaTime * 1000.0f;
	if (passData.sceneTarget.IsValid())
	{
		const auto& size = passData.sceneTarget.GetTexture().GetDescriptor().size;
		std::snprintf(frameText, sizeof(frameText), "%.2f ms \xC2\xB7 %ux%u", frameTime, static_cast<uint32_t>(size.width), static_cast<uint32_t>(size.height));
	}
	else
	{
		std::snprintf(frameText, sizeof(frameText), "%.2f ms", frameTime);
	}

	ImGui::PushFont(mFonts->mono);
	const float lineHeight = ImGui::GetFontSize();
	const float textWidth = ImMax(ImGui::CalcTextSize(pipelineText).x, ImGui::CalcTextSize(frameText).x);
	const ImVec2 max(contentMin.x + contentSize.x - kMargin, contentMin.y + contentSize.y - kMargin);
	const ImVec2 min(max.x - textWidth - kPadding.x * 2.0f, max.y - lineHeight * 2.0f - kLineGap - kPadding.y * 2.0f);

	auto drawList = ImGui::GetWindowDrawList();
	drawList->AddRectFilled(min, max, Widgets::ColorU32(Gleam::Theme::Background, 0.85f), 4.0f);
	drawList->AddRect(min, max, Widgets::ColorU32(Gleam::Theme::Border), 4.0f);
	drawList->AddText(ImVec2(min.x + kPadding.x, min.y + kPadding.y), Widgets::ColorU32(Gleam::Theme::TextBadge), pipelineText);
	drawList->AddText(ImVec2(min.x + kPadding.x, min.y + kPadding.y + lineHeight + kLineGap), Widgets::ColorU32(Gleam::Theme::TextMuted), frameText);
	ImGui::PopFont();
}

void WorldViewport::DrawViewport(Gleam::ImGuiRenderer* imgui, const Gleam::ImGuiPassData& passData)
{
	float displayScale = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>()->GetDisplayScale();
	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	if (mViewportSize != Gleam::Size(viewportSize.x, viewportSize.y))
	{
		mViewportSize.width = viewportSize.x;
		mViewportSize.height = viewportSize.y;
		mViewportSizeChanged = true;
	}

	if (passData.sceneTarget.IsValid() == false)
	{
		constexpr const char* kNoCameraText = "No active camera";
		const ImVec2 textSize = ImGui::CalcTextSize(kNoCameraText);
		const ImVec2 cursor = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(cursor.x + (viewportSize.x - textSize.x) * 0.5f, cursor.y + (viewportSize.y - textSize.y) * 0.5f));
		ImGui::TextDisabled("%s", kNoCameraText);
		return;
	}

	const auto& sceneRTsize = passData.sceneTarget.GetTexture().GetDescriptor().size;
	ImGui::Image(imgui->GetImTextureIDForTexture(passData.sceneTarget), ImVec2(sceneRTsize.width / displayScale, sceneRTsize.height / displayScale), ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));

	ImVec2 imageMin = ImGui::GetItemRectMin();
	ImVec2 imageSize = ImGui::GetItemRectSize();
	bool viewportHovered = ImGui::IsItemHovered() and mToolbarHovered == false;

	const Gleam::Float2 rectMin(imageMin.x, imageMin.y);
	const Gleam::Float2 rectSize(imageSize.x, imageSize.y);

	if (mWorldManager->IsSimulating())
	{
		const auto& borderColor = mWorldManager->GetPlayState() == PlayState::Paused ? Gleam::Theme::Warning : Gleam::Theme::Accent;
		ImGui::GetWindowDrawList()->AddRect(imageMin, ImVec2(imageMin.x + imageSize.x, imageMin.y + imageSize.y), Widgets::ColorU32(borderColor), 0.0f, 0, 2.0f);
		return;
	}

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
			mUndoSystem->RecordEntityCreation(entity);
		}
		ImGui::EndDragDropTarget();
	}
}

void WorldViewport::UpdateSelectionMarquee(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize, const Gleam::Size& targetSize, bool viewportHovered)
{
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
		// Below this the drag is still a click, so a small wobble does not turn into a rectangle
		constexpr float kMarqueeThreshold = 4.0f;
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
	mSelectionSystem->RequestPick(request, additive ? SelectionMode::Toggle : SelectionMode::Replace);

	mMarqueeDragging = false;
	mMarqueeMoved = false;
}

Gleam::TArray<Gleam::EntityHandle> WorldViewport::GatherGizmoTargets(const Gleam::EntityManager& entityManager) const
{
	Gleam::TArray<Gleam::EntityHandle> gizmoTargets;
	for (auto handle : mSelectionSystem->GetSelectedEntities())
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
			if (mSelectionSystem->IsSelected(parent))
			{
				ancestorSelected = true;
				break;
			}
			parent = entityManager.GetComponent<Gleam::Entity>(parent).GetParent();
		}

		if (ancestorSelected == false)
		{
			gizmoTargets.push_back(handle);
		}
	}
	return gizmoTargets;
}

void WorldViewport::DrawTransformGizmo(const Gleam::Float2& imageMin, const Gleam::Float2& imageSize)
{
	const auto& entityManager = mEditWorld->GetEntityManager();
	auto gizmoTargets = GatherGizmoTargets(entityManager);
	if (gizmoTargets.empty())
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

	auto pivot = GetPivotTransform(entityManager, gizmoTargets, mSelectionSystem->GetActiveEntity());
	const auto startPivot = pivot;

	const bool wasDragging = mTransformGizmo.IsDragging();
	const bool inputEnabled = ImGui::IsWindowHovered() && mCursorVisible && mToolbarHovered == false;
	if (mTransformGizmo.Manipulate(viewport, inputEnabled, pivot))
	{
		ApplyPivotDelta(mEditWorld->GetEntityManager(), gizmoTargets, startPivot, pivot);
	}

	if (wasDragging == false && mTransformGizmo.IsDragging())
	{
		mUndoSystem->BeginTransformTransaction(gizmoTargets);
	}
	else if (wasDragging && mTransformGizmo.IsDragging() == false)
	{
		mUndoSystem->EndTransaction();
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

	mViewportWorld = mWorldManager->GetPlayWorld();
	if (mViewportWorld)
	{
		mViewportWorld->GetEntityManager().ForEach<Gleam::Camera>([&](Gleam::Camera& gameCamera)
		{
			gameCamera.SetViewport(mViewportSize * displayScale);
		});
	}
}
