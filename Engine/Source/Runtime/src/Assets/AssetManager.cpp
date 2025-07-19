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

	Filesystem::ForEach(Globals::ProjectContentDirectory, [this](const auto& entry)
	{
		if (entry.Extension() == Asset::Extension() ||
			entry.Extension() == Prefab::Extension() ||
			entry.Extension() == World::Extension())
		{
			EmplaceAssetPath(entry);
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

		auto relPath = Filesystem::Relative(path, Globals::ProjectContentDirectory);
		std::lock_guard<std::mutex> lock(mMutex);
		switch (event)
		{
			case FileWatchEvent::Added:
			{
				EmplaceAssetPath(relPath);
				break;
			}
			case FileWatchEvent::Removed:
			{
				auto it = std::find_if(mAssetPaths.begin(), mAssetPaths.end(), [&](auto pair)
				{
					return pair.second == relPath;
				});

				if (it != mAssetPaths.end())
				{
					mAssetPaths.erase(it);
				}
				break;
			}
			case FileWatchEvent::Modified:
			{
				auto it = std::find_if(mAssetPaths.begin(), mAssetPaths.end(), [&](auto pair)
				{
					return pair.second == relPath;
				});

				if (it == mAssetPaths.end())
				{
					EmplaceAssetPath(relPath);
				}
				break;
			}
			default: break;
		}
	});
}

void AssetManager::Shutdown()
{
	for (auto& [ref, asset] : mAssetCache)
	{
		asset->Release();
	}
	mAssetCache.clear();
	mAssetPaths.clear();
}

void AssetManager::EmplaceAssetPath(const Path& path)
{
	Guid guid = TString(path.Stem());
	auto relPath = path.IsRelative() ? path : Filesystem::Relative(path, Globals::ProjectContentDirectory);

	if (guid != Guid::InvalidGuid())
	{
		AssetReference assetRef = { .guid = guid };
		mAssetPaths[assetRef] = relPath;
	}
}

const Path& AssetManager::GetAssetPath(const AssetReference& ref) const
{
	auto it = mAssetPaths.find(ref);
	if (it != mAssetPaths.end())
	{
		return it->second;
	}

	GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", ref.guid.ToString());
	GLEAM_ASSERT(false);
	static Path invalidPath;
	return invalidPath;
}
