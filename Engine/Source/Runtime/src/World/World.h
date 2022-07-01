#pragma once
#include "EntityManager.h"

namespace Gleam {

class World final
{    
public:

	World(const TString& name)
		: mName(name)
	{

	}

	const EntityManager& GetEntityManager() const
	{
		return mEntityManager;
	}

private:

	TString mName;
	EntityManager mEntityManager;

};

} // namespace Gleam
