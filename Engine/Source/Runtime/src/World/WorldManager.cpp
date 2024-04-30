#include "gpch.h"
#include "WorldManager.h"

using namespace Gleam;

void WorldManager::Initialize()
{

}

void WorldManager::Shutdown()
{
	mLoadedWorlds.clear();
}

void WorldManager::Configure(const WorldConfig& config)
{
	mWorldsInBuild = config.worlds;
	mActiveWorld = config.startingWorldIndex;
	LoadWorld(config.startingWorldIndex);
}

size_t WorldManager::LoadWorld(uint32_t buildIndex)
{
	auto index = mLoadedWorlds.size();
	// TODO: Load from world asset file
	mLoadedWorlds.emplace_back(CreateScope<World>("Starter World"));
	return index;
}

void WorldManager::UnloadWorld(uint32_t index)
{
	if (index >= mLoadedWorlds.size())
	{
		GLEAM_CORE_ERROR("World id: {0} is not loaded", index);
		return;
	}
	mLoadedWorlds.erase(mLoadedWorlds.begin() + index);
}

World* WorldManager::GetWorld(uint32_t index)
{
	if (index >= mLoadedWorlds.size())
	{
		GLEAM_CORE_ERROR("World id: {0} is not loaded", index);
		return GetActiveWorld();
	}
	return mLoadedWorlds[index].get();
}

World* WorldManager::GetActiveWorld()
{
	GLEAM_ASSERT(mActiveWorld < mLoadedWorlds.size(), "Active world index is out of bounds");
	return mLoadedWorlds[mActiveWorld].get();
}
