#pragma once
#include "EntityManager.h"

namespace Gleam {

class World;

class ComponentSystem
{
    friend class World;
    
public:
    
    virtual ~ComponentSystem() = default;

protected:

	virtual void OnCreate(EntityManager& entityManager) {};

	virtual void OnUpdate(EntityManager& entityManager) {};

	virtual void OnFixedUpdate(EntityManager& entityManager) {};

	virtual void OnDestroy(EntityManager& entityManager) {};
    
    bool enabled = true;

};

} // namespace Gleam
