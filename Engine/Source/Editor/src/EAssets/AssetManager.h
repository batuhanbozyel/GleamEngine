#pragma once
#include "Gleam.h"
#include "AssetType.h"
#include "AssetPackage.h"

namespace GEditor {

class AssetManager final
{
public:

    AssetManager() = default;
	AssetManager(const Gleam::Filesystem::path& directory);

    void Import(const Gleam::Filesystem::path& directory, const AssetPackage& package);
    
    AssetType GetAssetType(const Gleam::Asset& asset) const;

private:

	Gleam::Filesystem::path mAssetDirectory;
    
    Gleam::HashMap<Gleam::Asset, Gleam::Filesystem::path> mAssetCache;

};

} // namespace GEditor
