//
//  RendererVisualizationSystem.cpp
//  Editor
//

#include "RendererVisualizationSystem.h"
#include "Selection/SelectionSystem.h"
#include "View/Panels/World/EditorCamera.h"

#include <Editor.Reflection.generated.h>

#include "Core/Globals.h"
#include "Core/Engine.h"

#include "Math/Float4x4.h"

#include "Renderer/Mesh.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/Renderers/DebugRenderer.h"

#include "World/EntityManager.h"
#include "World/Components/Camera.h"
#include "World/Systems/RenderSceneProxy.h"

using namespace GEditor;

static constexpr Gleam::Color32 kFrustumColor = Gleam::Color32(200, 200, 200, 255);
static constexpr Gleam::Color32 kSelectedFrustumColor = Gleam::Color32(255, 200, 80, 255);
static constexpr Gleam::Color32 kMeshBoundsColor = Gleam::Color32(120, 200, 255, 255);

static void DrawFrustum(Gleam::DebugRenderer* renderer, const Gleam::Entity& entity, const Gleam::Camera& camera, Gleam::Color32 color)
{
	// Same matrices as RenderSystem::SetupCameraRenderData, so the lines match what the camera renders
	Gleam::Float4x4 projection;
	if (camera.projectionType == Gleam::ProjectionType::Perspective)
	{
		projection = Gleam::Float4x4::Perspective(Gleam::Math::Deg2Rad(camera.fov), camera.aspectRatio, camera.nearPlane, camera.farPlane);
	}
	else
	{
		const auto resolution = camera.GetViewport();
		projection = Gleam::Float4x4::Ortho(resolution.width, resolution.height, camera.nearPlane, camera.farPlane);
	}
	const auto view = Gleam::Float4x4::LookTo(entity.GetWorldPosition(), entity.ForwardVector(), entity.UpVector());
	const auto invViewProjection = Gleam::Math::Inverse(projection * view);

	// Bit 0: x, bit 1: y, bit 2: near (z = 0) or far (z = 1)
	Gleam::Float3 corners[8];
	for (uint32_t i = 0; i < 8; ++i)
	{
		const Gleam::Float3 ndc{ (i & 1) ? 1.0f : -1.0f, (i & 2) ? 1.0f : -1.0f, (i & 4) ? 1.0f : 0.0f };
		corners[i] = invViewProjection * ndc;
	}

	constexpr uint32_t kEdges[12][2] = {
		{ 0, 1 }, { 1, 3 }, { 3, 2 }, { 2, 0 },
		{ 4, 5 }, { 5, 7 }, { 7, 6 }, { 6, 4 },
		{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
	};
	for (const auto& edge : kEdges)
	{
		renderer->DrawLine(corners[edge[0]], corners[edge[1]], color, false);
	}
}

RendererVisualizationSystem::RendererVisualizationSystem(const SelectionSystem* selectionSystem, const Gleam::RenderSceneProxy* sceneProxy)
	: mSelectionSystem(selectionSystem)
	, mSceneProxy(sceneProxy)
{

}

void RendererVisualizationSystem::OnUpdate(Gleam::EntityManager& entityManager)
{
	static auto renderSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::RenderSystem>();
	auto renderer = renderSystem->GetRenderPipeline(Gleam::RenderPath::Default)->GetRenderer<Gleam::DebugRenderer>();

	const bool drawAllFrustums = mSettings.flags.Has(Gleam::RendererVisualizationFlag::CameraFrustums);
	entityManager.ForEach<Gleam::Entity, Gleam::Camera>([&](Gleam::EntityHandle handle, const Gleam::Entity& entity, const Gleam::Camera& camera)
	{
		const bool selected = mSelectionSystem->IsSelected(handle);
		if (entity.IsActive() and (selected or drawAllFrustums))
		{
			DrawFrustum(renderer, entity, camera, selected ? kSelectedFrustumColor : kFrustumColor);
		}
	}, Gleam::Exclude<Gleam::EditorCamera>{});

	if (mSettings.flags.Has(Gleam::RendererVisualizationFlag::MeshBounds))
	{
		for (const auto& record : mSceneProxy->GetRecords())
		{
			if (record.mesh and record.mesh->GetBounds().IsValid())
			{
				// The record's transform is refreshed after the world update, so it would trail a moving entity by a frame
				const auto& entity = entityManager.GetComponent<Gleam::Entity>(record.entity);
				const auto transform = Gleam::Float4x4::TRS(entity.GetWorldPosition(), entity.GetWorldRotation(), entity.GetWorldScale());
				renderer->DrawBoundingBox(record.mesh->GetBounds(), transform, kMeshBoundsColor, false);
			}
		}
	}
}
