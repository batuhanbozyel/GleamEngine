#include "gpch.h"
#include "AssetManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"

#include "IO/Log.h"
#include "IO/FileWatcher.h"

#include "World/World.h"
#include "World/Prefab.h"

#include "Renderer/Mesh.h"
#include "Renderer/Texture2D.h"
#include "Renderer/Material/Material.h"
#include "Renderer/Material/MaterialInstance.h"

using namespace Gleam;

void AssetManager::Initialize(Application* app)
{
	RegisterMetaAsset<Mesh, MeshDescriptor>();
	RegisterMetaAsset<Material, MaterialDescriptor>();
	RegisterMetaAsset<Texture2D, Texture2DDescriptor>();
	RegisterMetaAsset<MaterialInstance, MaterialInstanceDescriptor>();

	mStorage = CreateScope<AssetStorage>(Globals::ProjectContentDirectory);

	Filesystem::ForEach(Globals::ProjectContentDirectory, [this](const auto& entry)
	{
		if (entry.Extension() == Asset::Extension() ||
			entry.Extension() == Prefab::Extension() ||
			entry.Extension() == World::Extension())
		{
			mStorage->EmplaceAssetPath(entry);
		}
	}, true);

	auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
	fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Path& path, FileWatchEvent event)
	{
		if (path.Extension() != Asset::Extension() &&
			path.Extension() != Prefab::Extension() &&
			path.Extension() != World::Extension())
		{
			return;
		}

		switch (event)
		{
			case FileWatchEvent::Added:
			case FileWatchEvent::Modified:
			{
				mStorage->EmplaceAssetPath(path);
				break;
			}
			case FileWatchEvent::Removed:
			{
				mStorage->RemoveAssetPath(path);
				break;
			}
			default: break;
		}
	});
}

void AssetManager::Shutdown(Application* app)
{
	// TODO: Fix me!!!
	// There are assets depending other assets such as MaterialInstance -> Material
	// We need proper deallocation logic for such dependency
	mAssetCache.clear();
	mStorage.reset();
}

const Path& AssetManager::GetAssetPath(const AssetReference& ref) const
{
	return mStorage->GetAssetPath(ref);
}
