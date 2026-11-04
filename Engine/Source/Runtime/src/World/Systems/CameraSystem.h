#pragma once
#include "World/ComponentSystem.h"
#include "World/Entity.h"

namespace Gleam {

class CameraSystem final : public ComponentSystem
{
public:

	EntityHandle GetMainCamera(const EntityManager& entityManager) const;

};

} // namespace Gleam
