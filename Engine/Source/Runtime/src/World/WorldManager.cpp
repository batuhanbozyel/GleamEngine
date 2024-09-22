#include "gpch.h"
#include "WorldManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "IO/FileWatcher.h"

using namespace Gleam;

void WorldManager::Initialize(Application* app)
{
	Filesystem::ForEach(Globals::ProjectContentDirectory, [this](const auto& entry)
	{
		if (entry.extension() == World::Extension())
		{
			auto guid = Guid(entry.stem().string());
			auto path = Filesystem::Relative(entry, Globals::ProjectContentDirectory);

			AssetReference assetRef = { .guid = guid };
			mWorldPaths.emplace(assetRef, path);
		}
	}, true);

	auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
	fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Filesystem::Path& path, FileWatchEvent event)
	{
		if (path.extension() == World::Extension() && event == FileWatchEvent::Added)
		{
			std::lock_guard<std::mutex> lock(mMutex);

			auto guid = Guid(path.stem().string());
			auto relPath = Filesystem::Relative(path, Globals::ProjectContentDirectory);

			AssetReference assetRef = { .guid = guid };
			mWorldPaths.emplace(assetRef, relPath);
		}
	});
}

void WorldManager::Shutdown()
{
	mWorldPaths.clear();
	mLoadedWorlds.clear();
	mWorldsInBuild.clear();
}

void WorldManager::Configure(const WorldConfig& config)
{
	mWorldsInBuild = config.worlds;
	mActiveWorld = config.startingWorldIndex;
	LoadWorld(config.startingWorldIndex);
}

size_t WorldManager::LoadWorld(uint32_t buildIndex)
{
	const auto& worldRef = mWorldsInBuild[buildIndex];
	auto worldFile = Globals::ProjectContentDirectory/mWorldPaths[worldRef];
	auto file = Filesystem::Open(worldFile, FileType::Text);

	auto world = CreateScope<World>();
	world->Deserialize(file.GetStream());

	auto index = mLoadedWorlds.size();
	mLoadedWorlds.emplace_back(std::move(world));
	return index;
}

void WorldManager::UnloadWorld(uint32_t index)
{
	if (index >= mLoadedWorlds.size())
	{
		GLEAM_CORE_ERROR("World is not loaded: {0}", index);
		return;
	}
	mLoadedWorlds.erase(mLoadedWorlds.begin() + index);
}

World* WorldManager::GetWorld(uint32_t index)
{
	if (index >= mLoadedWorlds.size())
	{
		GLEAM_CORE_ERROR("World is not loaded: {0}", index);
		return GetActiveWorld();
	}
	return mLoadedWorlds[index].get();
}

World* WorldManager::GetActiveWorld()
{
	GLEAM_ASSERT(mActiveWorld < mLoadedWorlds.size(), "Active world index is out of bounds");
	return mLoadedWorlds[mActiveWorld].get();
}
