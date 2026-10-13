//
//  RendererVisualizationSystem.h
//  Editor
//

#pragma once
#include "RendererVisualization.h"

#include "World/ComponentSystem.h"

namespace Gleam {
class RenderSceneProxy;
} // namespace Gleam

namespace GEditor {

class SelectionSystem;

class RendererVisualizationSystem final : public Gleam::ComponentSystem
{
public:

	RendererVisualizationSystem(const SelectionSystem* selectionSystem, const Gleam::RenderSceneProxy* sceneProxy);

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

	const SelectionSystem* mSelectionSystem = nullptr;

	const Gleam::RenderSceneProxy* mSceneProxy = nullptr;

};

} // namespace GEditor
