#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker;
class AssetRegistry;

struct AssetPackage
{
	Gleam::TArray<Gleam::RefCounted<AssetBaker>> bakers;
	AssetRegistry* registry;

	AssetPackage(AssetRegistry* registry)
		: registry(registry)
	{

	}

	virtual ~AssetPackage() = default;
};

#define AssetPackageType(type) type(AssetRegistry* registry) : AssetPackage(registry) {}

} // namespace GEditor
