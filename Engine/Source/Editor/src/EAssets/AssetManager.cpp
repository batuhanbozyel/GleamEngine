#include "Gleam.h"
#include "AssetManager.h"
#include "AssetBaker.h"

using namespace GEditor;

AssetManager::AssetManager(const Gleam::Filesystem::path& directory)
	: mAssetDirectory(directory)
{
	
}

void AssetManager::Import(const Gleam::Filesystem::path& directory, const AssetPackage& package)
{
	for (auto& baker : package.bakers)
	{
		auto asset = baker->Bake(directory);
        auto path = directory/baker->Filename();
		mAssetCache.insert({ asset, path });
	}
}
