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

#include "Input/InputActionAsset.h"

#include <EASTL/algorithm.h>

using namespace Gleam;

void AssetManager::Initialize(Application* app)
{
	RegisterMetaAsset<Mesh, MeshDescriptor>();
	RegisterMetaAsset<Material, MaterialDescriptor>();
	RegisterMetaAsset<Texture2D, Texture2DDescriptor>();
	RegisterMetaAsset<MaterialInstance, MaterialInstanceDescriptor>();
	RegisterMetaAsset<InputActionAsset, InputActionAssetDescriptor>();

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

void AssetManager::Tick(Application* app)
{
	for (const auto& ref : mPendingReloads)
	{
		auto it = mAssetCache.find(ref);
		if (it == mAssetCache.end())
		{
			continue;
		}

		auto asset = it->second.asset.get();
		const uint32_t refCount = asset->mRefCount;
		const uint32_t version = asset->mVersion;

		asset->~Asset();
		entt::resolve(it->second.type).func("ConstructAsset"_hs).invoke({}, asset, ref);

		asset->mRefCount = refCount;
		asset->mVersion = version + 1;
	}
	mPendingReloads.clear();

	// Walks by index because erasing an asset releases its dependencies, which appends to the list mid-loop.
	for (size_t i = 0; i < mPendingEvictions.size(); ++i)
	{
		const auto ref = mPendingEvictions[i];
		auto it = mAssetCache.find(ref);
		if (it != mAssetCache.end() and it->second.asset->mRefCount == 0)
		{
			mStorage->CloseAssetFile(ref);
			mAssetCache.erase(it);
		}
	}
	mPendingEvictions.clear();
}

void AssetManager::Reload(const AssetReference& ref)
{
	if (eastl::find(mPendingReloads.begin(), mPendingReloads.end(), ref) == mPendingReloads.end())
	{
		mPendingReloads.push_back(ref);
	}
}

const Path& AssetManager::GetAssetPath(const AssetReference& ref) const
{
	return mStorage->GetAssetPath(ref);
}
