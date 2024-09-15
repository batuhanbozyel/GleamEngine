#pragma once
#include "Gleam.h"
#include "AssetPackage.h"

namespace GEditor {

struct AssetItem
{
    Gleam::AssetReference reference;
    Gleam::Guid type;
    Gleam::TString name;
};

class AssetRegistry final : public Gleam::WorldSubsystem
{
public:

	AssetRegistry(const Gleam::Filesystem::Path& directory);

	virtual void Initialize(Gleam::World* world) override;

	virtual void Shutdown() override;

	void Import(const Gleam::Filesystem::Path& directory, const AssetPackage& package);

	template<typename T>
	const AssetItem& RegisterAsset(const Gleam::Filesystem::Path& path)
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		return RegisterAsset(path, type);
	}

	template<typename T>
	const AssetItem& GetAsset(const Gleam::Filesystem::Path& path) const
	{
		const auto& type = Gleam::Reflection::GetClass<T>().Guid();
		auto relPath = path.is_relative() ? path : Gleam::Filesystem::Relative(path, mAssetDirectory);
		auto it = mAssetCache.find(relPath);
		if (it != mAssetCache.end())
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
		GLEAM_ASSERT(false);
		static AssetItem invalidAsset;
		return invalidAsset;
	}

	const AssetItem& GetAsset(const Gleam::Guid& guid) const;

private:

	const AssetItem& RegisterAsset(const Gleam::Filesystem::Path& path, const Gleam::Guid& type);

	Gleam::Filesystem::Path mAssetDirectory;
    
    Gleam::HashMap<Gleam::Filesystem::Path, Gleam::TArray<AssetItem>> mAssetCache;

};

} // namespace GEditor
