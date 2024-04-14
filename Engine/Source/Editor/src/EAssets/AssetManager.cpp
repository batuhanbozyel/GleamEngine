#include "Gleam.h"
#include "AssetManager.h"
#include "AssetBaker.h"

using namespace GEditor;

EAssetManager::EAssetManager(const Gleam::Filesystem::path& directory)
	: mAssetDirectory(directory)
{
	
}

void EAssetManager::Import(const Gleam::Filesystem::path& directory, const AssetPackage& package)
{
	for (auto& baker : package.bakers)
	{
		auto asset = baker->Bake(directory);
        auto path = directory/baker->Filename();
		mAssetCache.insert({ path, asset });
        GLEAM_INFO("Asset imported: {0} GUID: {1}", baker->Filename(), asset.GetGuid().ToString());
	}
}

const Gleam::Asset& EAssetManager::GetAsset(const Gleam::Filesystem::path& path) const
{
	auto it = mAssetCache.find(path);
	if (it != mAssetCache.end())
	{
		return it->second;
	}
	static Gleam::Asset invalidAsset;
	return invalidAsset;
}
