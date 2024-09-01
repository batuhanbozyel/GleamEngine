#include "Gleam.h"
#include "AssetRegistry.h"
#include "AssetBaker.h"
#include "MaterialSource.h"

using namespace GEditor;

AssetRegistry::AssetRegistry(const Gleam::Filesystem::Path& directory)
	: mAssetDirectory(directory)
{
    Gleam::Filesystem::ForEach(directory, [this](const auto& entry)
    {
        if (entry.extension() == ".mat")
        {
            auto materialSource = MaterialSource();
            auto settings = MaterialSource::ImportSettings();
            materialSource.Import(entry, settings);
        }
    }, true);
}

void AssetRegistry::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.bakers)
	{
		auto asset = baker->Bake(directory);
        auto path = directory/baker->Filename();
		mAssetCache.insert({ path, asset });
        GLEAM_INFO("Asset imported: {0} GUID: {1}", baker->Filename(), asset.guid.ToString());
	}
}

const Gleam::AssetReference& AssetRegistry::GetAsset(const Gleam::Filesystem::Path& path) const
{
	auto it = mAssetCache.find(path);
	if (it != mAssetCache.end())
	{
		return it->second;
	}
	static Gleam::AssetReference invalidAsset;
	return invalidAsset;
}
