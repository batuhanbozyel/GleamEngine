#include "gpch.h"
#include "AssetLibrary.h"
#include "ModelImporter.h"

using namespace Gleam;

Filesystem::path AssetLibrary::GetDefaultAssetPath()
{
	return Filesystem::current_path().append("Assets/");
}

const Model& AssetLibrary::ImportModel(const Filesystem::path& path)
{
    auto modelCacheIt = sModelCache.find(path);
    if (modelCacheIt != sModelCache.end())
    {
        return modelCacheIt->second;
    }

    return sModelCache.insert(sModelCache.end(), { path, ModelImporter::Import(path) })->second;
}
