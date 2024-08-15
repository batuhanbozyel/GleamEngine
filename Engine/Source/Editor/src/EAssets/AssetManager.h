#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

class EAssetManager final
{
public:

    EAssetManager() = default;
	EAssetManager(const Gleam::Filesystem::Path& directory);

	void Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package);

	const Gleam::AssetReference& GetAsset(const Gleam::Filesystem::Path& path) const;

private:

	Gleam::Filesystem::Path mAssetDirectory;
    
    Gleam::HashMap<Gleam::Filesystem::Path, Gleam::AssetReference> mAssetCache;

};

} // namespace GEditor
