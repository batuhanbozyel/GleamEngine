#include "gpch.h"
#include "AssetManager.h"

using namespace Gleam;

void AssetManager::Initialize()
{
    
}

void AssetManager::Shutdown()
{
    mAssets.clear();
}

void AssetManager::Reload(const Filesystem::path& directory)
{
    
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
