//
//  PhysicsVisualizationSystem.h
//  Editor
//

#pragma once
#include "PhysicsVisualization.h"
#include "EAssets/Cookers/PhysicsCooker.h"

#include "World/ComponentSystem.h"
#include "Assets/AssetReference.h"
#include "Renderer/Buffer.h"
#include "Container/Hash.h"

namespace Gleam {
struct RigidBody;
} // namespace Gleam

namespace GEditor {

struct ColliderWireMesh
{
	Gleam::Buffer buffer;
	uint32_t firstIndex = 0;
	uint32_t indexCount = 0;
};

using ColliderWireMeshCache = Gleam::HashMap<Gleam::AssetReference, ColliderWireMesh>;

class PhysicsVisualizationSystem final : public Gleam::ComponentSystem
{
public:

	virtual void OnUpdate(Gleam::EntityManager& entityManager) override;

	virtual void OnDestroy(Gleam::EntityManager& entityManager) override;

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

	const ColliderWireMesh& GetConvexWireMesh(const Gleam::AssetReference& ref);

	const ColliderWireMesh& GetTriangleWireMesh(const Gleam::AssetReference& ref);

	Gleam::PhysicsVisualizationSettings mSettings;

	Gleam::Scope<PhysicsCooker> mCooker = PhysicsCooker::Create();

	ColliderWireMeshCache mConvexWireMeshes;

	ColliderWireMeshCache mTriangleWireMeshes;

};

} // namespace GEditor
