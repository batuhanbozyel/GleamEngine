#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker;
class AssetRegistry;

struct AssetPackage
{
	Gleam::TArray<Gleam::RefCounted<AssetBaker>> bakers;
	const AssetRegistry* registry;

	AssetPackage(const AssetRegistry* registry)
		: registry(registry)
	{

	}

	virtual ~AssetPackage() = default;
};

#define AssetPackageType(type) type(const AssetRegistry* registry) : AssetPackage(registry) {}

} // namespace GEditor
