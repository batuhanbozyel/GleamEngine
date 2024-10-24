#include "gpch.h"
#include "AssetManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "IO/FileWatcher.h"

using namespace Gleam;

void AssetManager::Initialize(Application* app)
{
	Filesystem::ForEach(Globals::ProjectContentDirectory, [this](const auto& entry)
	{
		if (entry.extension() == ".asset")
		{
			auto guid = Guid(entry.stem().string());
			auto relPath = Filesystem::Relative(entry, Globals::ProjectContentDirectory);

			Asset asset{ .path = relPath };
			AssetReference assetRef = { .guid = guid };
			mAssets.emplace(assetRef, asset);
		}
	}, true);

    auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
    fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Filesystem::Path& path, FileWatchEvent event)
    {
        if (path.extension() != Asset::extension())
        {
            return;
        }
        
        Asset asset{ .path = Filesystem::Relative(path, Globals::ProjectContentDirectory) };

		std::lock_guard<std::mutex> lock(mMutex);
        switch (event)
        {
            case FileWatchEvent::Added:
            {
				TryEmplaceAsset(asset);
                break;
            }
            case FileWatchEvent::Removed:
            {
                auto it = std::find_if(mAssets.begin(), mAssets.end(), [&](auto pair)
                {
                    return pair.second == asset;
                });
                
                if (it != mAssets.end())
                {
                    mAssets.erase(it);
                }
                break;
            }
			case FileWatchEvent::Modified:
			{
				auto it = std::find_if(mAssets.begin(), mAssets.end(), [&](auto pair)
				{
					return pair.second == asset;
				});

				if (it == mAssets.end())
				{
					TryEmplaceAsset(asset);
				}
				break;
			}
            default: break;
        }
    });
}

void AssetManager::Shutdown()
{
    mAssets.clear();
}

bool AssetManager::TryEmplaceAsset(const Asset& asset)
{
	Guid guid = asset.path.stem().string();
	if (guid == Guid::InvalidGuid())
	{
		return false;
	}

	AssetReference assetRef = { .guid = guid };
	mAssets[assetRef] = asset;
	return true;
}
