#pragma once
#include "World/ComponentSystem.h"

namespace Gleam {

class TransformSystem : public ComponentSystem
{
public:

	virtual void OnUpdate(EntityManager& entityManager) override;

private:

	void UpdateTransform(EntityManager& entityManager, Transform& transform) const;

	bool RequiresUpdate(EntityManager& entityManager, const Transform& transform) const;

};

} // namespace Gleam