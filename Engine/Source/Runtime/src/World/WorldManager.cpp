#include "gpch.h"
#include "WorldManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "IO/FileWatcher.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

void WorldManager::Initialize(Application* app)
{
	
}

void WorldManager::Shutdown()
{
	mLoadedWorlds.clear();
	mWorldsInBuild.clear();
}

void WorldManager::Configure(const WorldConfig& config)
{
	mWorldsInBuild = config.worlds;
	mActiveWorld = config.startingWorldIndex;
}

void WorldManager::OpenWorld(uint32_t buildIndex)
{
	LoadWorld(buildIndex);
	mActiveWorld = buildIndex;
}

void WorldManager::LoadWorld(uint32_t buildIndex)
{
	const auto& worldRef = mWorldsInBuild[buildIndex];
	const auto& worldPath = Globals::GameInstance->GetSubsystem<AssetManager>()->GetAssetPath(worldRef);

	auto file = Filesystem::Open(Globals::ProjectContentDirectory / worldPath, FileType::Text);
	auto world = CreateScope<World>();

	world->Deserialize(file.GetStream());
	mLoadedWorlds.emplace(worldRef, std::move(world));
}

void WorldManager::SaveWorld(uint32_t buildIndex)
{
	const auto& worldRef = mWorldsInBuild[buildIndex];
	const auto& worldPath = Globals::GameInstance->GetSubsystem<AssetManager>()->GetAssetPath(worldRef);

	auto file = Filesystem::Create(Globals::ProjectContentDirectory / worldPath, FileType::Text);
	auto world = mLoadedWorlds[worldRef].get();

	world->Serialize(file.GetStream());
}

void WorldManager::SaveActiveWorld()
{
	SaveWorld(mActiveWorld);
}

World* WorldManager::GetActiveWorld()
{
	const auto& worldRef = mWorldsInBuild[mActiveWorld];
	auto it = mLoadedWorlds.find(worldRef);
	if (it != mLoadedWorlds.end())
	{
		return it->second.get();
	}
	return nullptr;
}
