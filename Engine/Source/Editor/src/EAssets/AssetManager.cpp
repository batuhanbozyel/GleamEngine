#include "Gleam.h"
#include "AssetManager.h"
#include "AssetBaker.h"

using namespace GEditor;

EAssetManager::EAssetManager(const Gleam::Filesystem::Path& directory)
	: mAssetDirectory(directory)
{
	
}

void EAssetManager::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.bakers)
	{
		auto asset = baker->Bake(directory);
        auto path = directory/baker->Filename();
		mAssetCache.insert({ path, asset });
        GLEAM_INFO("Asset imported: {0} GUID: {1}", baker->Filename(), asset.guid.ToString());
	}
}

const Gleam::AssetReference& EAssetManager::GetAsset(const Gleam::Filesystem::Path& path) const
{
	auto it = mAssetCache.find(path);
	if (it != mAssetCache.end())
	{
		return it->second;
	}
	static Gleam::AssetReference invalidAsset;
	return invalidAsset;
}
