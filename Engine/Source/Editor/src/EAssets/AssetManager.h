#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

class EAssetManager final
{
public:

    EAssetManager() = default;
	EAssetManager(const Gleam::Filesystem::path& directory);

	void Import(const Gleam::Filesystem::path& directory, const AssetPackage& package);

	const Gleam::AssetReference& GetAsset(const Gleam::Filesystem::path& path) const;

private:

	Gleam::Filesystem::path mAssetDirectory;
    
    Gleam::HashMap<Gleam::Filesystem::path, Gleam::AssetReference> mAssetCache;

};

} // namespace GEditor
