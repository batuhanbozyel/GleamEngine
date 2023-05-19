#include "gpch.h"
#include "World.h"

using namespace Gleam;

World::World(const TString& name)
	: mName(name)
{
    
}

void World::Update()
{
	for (auto system : mSystemManager)
	{
		system->OnUpdate(mEntityManager);
	}
}

void World::FixedUpdate()
{
	for (auto system : mSystemManager)
	{
		system->OnFixedUpdate(mEntityManager);
	}
}
