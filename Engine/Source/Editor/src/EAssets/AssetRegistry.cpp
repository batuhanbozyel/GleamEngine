#include "Gleam.h"
#include "AssetRegistry.h"
#include "AssetBaker.h"
#include "MaterialSource.h"

using namespace GEditor;

AssetRegistry::AssetRegistry(const Gleam::Filesystem::Path& directory)
	: mAssetDirectory(directory)
{
    // TODO: register all serialized assets on init
    // materials should only compile when first seen by the registry
    // existing materials should not change guid
    // it should only reimport if material/shader source changed since last compile
    Gleam::Filesystem::ForEach(directory, [this](const auto& entry)
    {
        if (entry.extension() == ".mat")
        {
            auto materialSource = MaterialSource(this);
            auto settings = MaterialSource::ImportSettings();
            materialSource.Import(entry, settings);
			Import(mAssetDirectory/"Materials", materialSource);
        }
    }, true);
}

void AssetRegistry::Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package)
{
	for (const auto& baker : package.bakers)
	{
		auto asset = baker->Bake(directory);
        auto path = directory/baker->Filename();
		auto relPath = Gleam::Filesystem::Relative(path, mAssetDirectory);
		mAssetCache.insert({ relPath, asset });
        GLEAM_INFO("Asset imported: {0} GUID: {1}", baker->Filename(), asset.guid.ToString());
	}
}

const Gleam::AssetReference& AssetRegistry::GetAsset(const Gleam::Filesystem::Path& path) const
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto it = mAssetCache.find(relPath);
	if (it != mAssetCache.end())
	{
		return it->second;
	}
	static Gleam::AssetReference invalidAsset;
	return invalidAsset;
}
