#include "gpch.h"
#include "World.h"
#include "Systems/RenderSceneProxy.h"
#include "Physics/PhysicsSystem.h"

using namespace Gleam;

World::World(const AssetReference& reference, const AssetHeader& header, const WorldDescriptor& descriptor)
	: Asset(reference, header)
	, mDescriptor(descriptor)
{
	Timestep::Reset();
	AddSubsystem<RenderSceneProxy>();
	AddSystem<PhysicsSystem>()->SetGravity(mDescriptor.gravity);
}

World::~World()
{
	for (int i = (int)mSystems.size() - 1; i >= 0; --i)
	{
		mSystems[i]->OnDestroy(mEntityManager);
	}
	mSystems.clear();
	mTickableSubsystems.clear();

	for (int i = (int)mSubsystems.size() - 1; i >= 0; --i)
	{
		mSubsystems[i]->Shutdown(this);
	}
	mSubsystems.clear();
}

void World::Update()
{
	Timestep::Step();
	for (auto subsystem : mTickableSubsystems)
	{
		subsystem->Tick(this);
	}

	while (Timestep::InFixedTimeStep())
	{
		Timestep::FixedStep();
		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnPreFixedUpdate(mEntityManager);
			}
		}

		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnFixedUpdate(mEntityManager);
			}
		}

		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnPostFixedUpdate(mEntityManager);
			}
		}
	}
	Timestep::Update();

	for (auto system : mSystems)
	{
		if (system->Enabled)
		{
			system->OnPreUpdate(mEntityManager);
		}
	}

	for (auto system : mSystems)
	{
		if (system->Enabled)
		{
			system->OnUpdate(mEntityManager);
		}
	}

	for (auto system : mSystems)
	{
		if (system->Enabled)
		{
			system->OnPostUpdate(mEntityManager);
		}
	}

}
