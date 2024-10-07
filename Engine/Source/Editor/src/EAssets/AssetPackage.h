#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker;
class AssetRegistry;
class EAssetManager;

struct AssetPackage
{
	Gleam::TArray<Gleam::RefCounted<AssetBaker>> mBakers;
	EAssetManager* mAssetManager;
	AssetRegistry* mRegistry;

	AssetPackage(EAssetManager* assetManager, AssetRegistry* registry)
		: mAssetManager(assetManager)
		, mRegistry(registry)
	{

	}

	virtual ~AssetPackage() = default;
};

#define AssetPackageType(type) type(EAssetManager* assetManager, AssetRegistry* registry) : AssetPackage(assetManager, registry) {}

} // namespace GEditor
