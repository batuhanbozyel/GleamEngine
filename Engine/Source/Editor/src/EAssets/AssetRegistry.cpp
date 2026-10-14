#include "AssetRegistry.h"

using namespace GEditor;

AssetRegistry::AssetRegistry(const Gleam::Path& directory)
	: mAssetDirectory(directory)
{

}

const AssetItem& AssetRegistry::RegisterAsset(const Gleam::Path& path, const Gleam::Guid& type)
{
	const auto& item = GetAsset(path, type);
	if (item.reference.guid == Gleam::Guid::InvalidGuid())
	{
		auto asset = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
		auto newItem = AssetItem{
			.reference = asset,
			.type = type,
			.name = path.Stem()
		};
		return RegisterAsset(path, newItem);
	}
	return item;
}

const AssetItem& AssetRegistry::RegisterAsset(const Gleam::Path& path, const AssetItem& item)
{
	auto relPath = path.IsRelative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	relPath.MakePreferred();

	auto& items = mAssets[relPath];
	auto it = std::find(items.begin(), items.end(), item);
	if (it != items.end())
	{
		return *it;
	}

	GLEAM_INFO("Asset imported: {0} GUID: {1}", item.name, item.reference.guid.ToString());
	const auto& registered = items.emplace_back(item);
	mAssetsByGuid[registered.reference.guid] = AssetLocation{
		.path = relPath,
		.index = static_cast<uint32_t>(items.size() - 1u)
	};
	return registered;
}

void AssetRegistry::UnregisterAsset(const Gleam::Guid& guid)
{
	auto it = mAssetsByGuid.find(guid);
	if (it == mAssetsByGuid.end())
	{
		return;
	}

	const auto location = it->second;
	mAssetsByGuid.erase(it);

	auto pathIt = mAssets.find(location.path);
	if (pathIt == mAssets.end())
	{
		return;
	}

	auto& items = pathIt->second;
	items.erase(items.begin() + location.index);
	for (uint32_t i = location.index; i < items.size(); ++i)
	{
		mAssetsByGuid[items[i].reference.guid].index = i;
	}

	if (items.empty())
	{
		mAssets.erase(pathIt);
	}
}

const AssetItem& AssetRegistry::GetAsset(const Gleam::Guid& guid) const
{
	const auto item = FindAsset(guid);
	if (item == nullptr)
	{
		GLEAM_ERROR("Asset could not located for GUID: {0}", guid.ToString());
		static AssetItem invalidAsset;
		return invalidAsset;
	}
	return *item;
}

const AssetItem* AssetRegistry::FindAsset(const Gleam::Guid& guid) const
{
	auto it = mAssetsByGuid.find(guid);
	if (it != mAssetsByGuid.end())
	{
		auto pathIt = mAssets.find(it->second.path);
		if (pathIt != mAssets.end())
		{
			return &pathIt->second[it->second.index];
		}
	}
	return nullptr;
}

const AssetItem& AssetRegistry::GetAsset(const Gleam::Path& path, const Gleam::Guid& type) const
{
	const auto item = FindAsset(path, type);
	if (item == nullptr)
	{
		auto relPath = path.IsRelative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
		relPath.MakePreferred();

		GLEAM_ERROR("Asset could not located for path: {0}", relPath.String());
		static AssetItem invalidAsset;
		return invalidAsset;
	}
	return *item;
}

const AssetItem* AssetRegistry::FindAsset(const Gleam::Path& path, const Gleam::Guid& type) const
{
	auto relPath = path.IsRelative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
	relPath.MakePreferred();

	auto it = mAssets.find(relPath);
	if (it != mAssets.end())
	{
		for (const auto& item : it->second)
		{
			if (item.type == type)
			{
				return &item;
			}
		}
	}
	return nullptr;
}
