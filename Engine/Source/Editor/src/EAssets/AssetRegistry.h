#pragma once
#include "Gleam.h"

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

	AssetRegistry(const Gleam::Filesystem::Path& directory);

	template<typename T>
	const AssetItem& RegisterAsset(const Gleam::Filesystem::Path& path)
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		return RegisterAsset(path, type);
	}

	const AssetItem& RegisterAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type);

	const AssetItem& RegisterAsset(const Gleam::Filesystem::Path& path, const AssetItem& item);

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Filesystem::Path& path) const
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		return GetAsset(path, type);
	}

	const AssetItem& GetAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type) const;

private:

	Gleam::Filesystem::Path mAssetDirectory;

	Gleam::HashMap<Gleam::Filesystem::Path, Gleam::TArray<AssetItem>> mAssets;
};

} // namespace GEditor
