#pragma once

namespace Gleam {

class World;
class EntityManager;

class ISystem
{
	friend class World;

protected:

	virtual void OnCreate(EntityManager& entityManager) {};

	virtual void OnUpdate(EntityManager& entityManager) {};

	virtual void OnFixedUpdate(EntityManager& entityManager) {};

	virtual void OnDestroy(EntityManager& entityManager) {};

};

} // namespace Gleam
