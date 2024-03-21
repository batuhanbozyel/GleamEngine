#pragma once
#include "Gleam.h"

namespace GEditor {

class AssetBaker;

struct AssetPackage
{
	Gleam::TArray<Gleam::RefCounted<AssetBaker>> bakers;

	virtual ~AssetPackage() = default;
};

} // namespace GEditor
