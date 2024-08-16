#include "gpch.h"
#include "AssetManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "IO/FileWatcher.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/BinarySerializer.h"

using namespace Gleam;

void AssetManager::Initialize()
{
    auto meta = Globals::ProjectContentDirectory/"Assets.meta";
    if (Filesystem::Exists(meta))
    {
        auto file = Filesystem::Open(meta, FileType::Binary);
        auto serializer = BinarySerializer(file.GetStream());
        mAssets = serializer.Deserialize<AssetReference, Asset>(file.GetSize());
    }
    
    auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
    fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Filesystem::Path& path, FileWatchEvent event)
    {
        if (path.extension() != Asset::extension())
        {
            return;
        }
        
        Asset asset{ .path = Filesystem::Relative(path, Globals::ProjectContentDirectory) };
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

const Asset& AssetManager::GetAsset(const AssetReference& asset) const
{
    auto it = mAssets.find(asset);
    if (it != mAssets.end())
    {
        return it->second;
    }
    
    GLEAM_CORE_ERROR("Asset could not located GUID: {0}, type: {1}", asset.guid.ToString(), asset.type.ToString());
    GLEAM_ASSERT(false);
    static Asset invalidAsset;
    return invalidAsset;
}

void AssetManager::TryEmplaceAsset(const Asset& asset)
{
	Guid guid = asset.path.stem().string();
	if (guid == Guid::InvalidGuid())
	{
		return;
	}

	auto file = Filesystem::Open(Globals::ProjectContentDirectory/asset.path, FileType::Text);
	if (not file.Empty())
	{
		auto serializer = JSONSerializer(file.GetStream());
		auto typeHeader = serializer.ParseHeader();

		AssetReference assetRef = { .type = typeHeader.guid, .guid = guid };

		std::lock_guard<std::mutex> lock(mEmplaceMutex);
		mAssets.emplace(assetRef, asset);
	}
}
