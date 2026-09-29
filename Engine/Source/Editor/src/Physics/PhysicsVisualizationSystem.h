//
//  PhysicsVisualizationSystem.h
//  Editor
//

#pragma once
#include "PhysicsVisualization.h"

#include "World/ComponentSystem.h"

namespace Gleam {
struct RigidBody;
} // namespace Gleam

namespace GEditor {

class PhysicsVisualizationSystem final : public Gleam::ComponentSystem
{
public:

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;

	const Gleam::PhysicsVisualizationSettings& GetSettings() const
	{
		return mSettings;
	}

	void SetSettings(const Gleam::PhysicsVisualizationSettings& settings)
	{
		mSettings = settings;
	}

private:

	bool IsVisible(const Gleam::RigidBody& rigidBody) const;

	Gleam::PhysicsVisualizationSettings mSettings;

};

} // namespace GEditor
