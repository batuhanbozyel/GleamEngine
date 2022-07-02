#pragma once
#include "EntityManager.h"

namespace Gleam {

class World final
{    
public:

	World(const TString& name = "World")
		: mName(name)
	{

	}

	EntityManager& GetEntityManager()
	{
		return mEntityManager;
	}

private:

	TString mName;
	EntityManager mEntityManager;

};

} // namespace Gleam
