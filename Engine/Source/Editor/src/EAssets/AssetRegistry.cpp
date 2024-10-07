#include "AssetRegistry.h"

using namespace GEditor;

AssetRegistry::AssetRegistry(const Gleam::Filesystem::Path& directory)
	: mAssetDirectory(directory)
{

}

const AssetItem& AssetRegistry::RegisterAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type)
{
	auto asset = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
	auto item = AssetItem{
		.reference = asset,
		.type = type,
		.name = path.stem().string()
	};
	return RegisterAsset(path, item);
}

const AssetItem& AssetRegistry::RegisterAsset(const Gleam::Filesystem::Path& path, const AssetItem& item)
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto& items = mAssets[relPath];
	auto it = std::find(items.begin(), items.end(), item);
	if (it != items.end())
	{
		return *it;
	}

	GLEAM_INFO("Asset imported: {0} GUID: {1}", item.name, item.reference.guid.ToString());
	return items.emplace_back(item);
}

const AssetItem& AssetRegistry::GetAsset(const Gleam::Guid& guid) const
{
	for (const auto& [path, items] : mAssets)
	{
		for (const auto& item : items)
		{
			if (item.reference.guid == guid)
			{
				return item;
			}
		}
	}

	GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", guid.ToString());
	static AssetItem invalidAsset;
	return invalidAsset;
}

const AssetItem& AssetRegistry::GetAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type) const
{
	auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	auto it = mAssets.find(relPath);
	if (it != mAssets.end())
	{
		for (const auto& item : it->second)
		{
			if (item.type == type)
			{
				return item;
			}
		}
	}

	GLEAM_ERROR("Asset could not located for path: {0}", relPath.string());
	static AssetItem invalidAsset;
	return invalidAsset;
}