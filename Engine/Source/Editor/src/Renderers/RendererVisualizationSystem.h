//
//  RendererVisualizationSystem.h
//  Editor
//

#pragma once
#include "RendererVisualization.h"

#include "World/ComponentSystem.h"

namespace Gleam {
class World;
class DebugRenderer;
class RenderSceneProxy;
} // namespace Gleam

namespace GEditor {

class SelectionSystem;

class RendererVisualizationSystem final : public Gleam::ComponentSystem
{
public:

	RendererVisualizationSystem(const Gleam::World* world, const SelectionSystem* selectionSystem, const Gleam::RenderSceneProxy* sceneProxy, Gleam::DebugRenderer* renderer);

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;

	const Gleam::RendererVisualizationSettings& GetSettings() const
	{
		return mSettings;
	}

	void SetSettings(const Gleam::RendererVisualizationSettings& settings)
	{
		mSettings = settings;
	}

private:

	Gleam::RendererVisualizationSettings mSettings;

	const Gleam::World* mWorld = nullptr;

	const SelectionSystem* mSelectionSystem = nullptr;

	const Gleam::RenderSceneProxy* mSceneProxy = nullptr;

	Gleam::DebugRenderer* mRenderer = nullptr;

};

} // namespace GEditor
