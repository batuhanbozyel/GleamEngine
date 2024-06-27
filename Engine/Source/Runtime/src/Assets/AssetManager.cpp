#include "gpch.h"
#include "AssetManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "IO/FileWatcher.h"
#include "Serialization/BinarySerializer.h"

using namespace Gleam;

void AssetManager::Initialize()
{
    auto meta = Globals::ProjectContentDirectory/"Assets.meta";
    if (Filesystem::exists(meta))
    {
        auto file = File(meta, FileType::Binary);
        auto serializer = BinarySerializer(file.GetStream());
        mAssets = serializer.Deserialize<AssetReference, Asset>(file.GetSize());
    }
    
    auto fileWatcher = Globals::Engine->GetSubsystem<FileWatcher>();
    fileWatcher->AddWatch(Globals::ProjectContentDirectory, [this](const Filesystem::path& path, FileWatchEvent event)
    {
        if (path.extension() != Asset::extension())
        {
            return;
        }
        
        Asset asset{ .path = Filesystem::relative(path, Globals::ProjectContentDirectory) };
        switch (event)
        {
            case FileWatchEvent::Added:
            {
                //mAssets.emplace_hint(mAssets.end(), { .type = ..., .guid = ... });
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
