#pragma once
#include "Assets/AssetReference.h"
#include "Core/GUID.h"
#include "IO/Path.h"

namespace GEditor {

struct AssetItem
{
    Gleam::AssetReference reference;
    Gleam::Guid type;
    Gleam::TString name;

	bool operator==(const AssetItem& other) const
	{
		return reference == other.reference
			&& type == other.type
			&& name == other.name;
	}
};

class AssetRegistry
{
public:

	AssetRegistry(const Gleam::Path& directory);

	template<typename T>
	const AssetItem& RegisterAsset(const Gleam::Path& path)
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		return RegisterAsset(path, type);
	}

	const AssetItem& RegisterAsset(const Gleam::Path& path, const Gleam::Guid& type);

	const AssetItem& RegisterAsset(const Gleam::Path& path, const AssetItem& item);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Path& path) const
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		return GetAsset(path, type);
	}

	const AssetItem& GetAsset(const Gleam::Path& path, const Gleam::Guid& type) const;

private:

	Gleam::Path mAssetDirectory;

	Gleam::HashMap<Gleam::Path, Gleam::TArray<AssetItem>> mAssets;
};

} // namespace GEditor
