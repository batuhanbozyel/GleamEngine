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
}

void WorldManager::OpenWorld(uint32_t buildIndex)
{
	LoadWorld(buildIndex);
	mActiveWorld = buildIndex;
}

void WorldManager::LoadWorld(uint32_t buildIndex)
{
	const auto& worldRef = mWorldsInBuild[buildIndex];
	auto worldFile = Globals::ProjectContentDirectory/mWorldPaths[worldRef];
	auto file = Filesystem::Open(worldFile, FileType::Text);

	auto world = CreateScope<World>();
	world->Deserialize(file.GetStream());
	mLoadedWorlds.emplace(worldRef, std::move(world));
}

void WorldManager::SaveWorld()
{
	const auto& worldRef = mWorldsInBuild[mActiveWorld];
	auto worldFile = Globals::ProjectContentDirectory / mWorldPaths[worldRef];
	auto file = Filesystem::Create(worldFile, FileType::Text);
	auto world = mLoadedWorlds[worldRef].get();
	world->Serialize(file.GetStream());
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
