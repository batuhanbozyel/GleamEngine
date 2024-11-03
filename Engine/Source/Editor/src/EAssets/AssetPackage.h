#pragma once
#include "Gleam.h"
#include "AssetBaker.h"
#include "AssetRegistry.h"

namespace GEditor {

class EAssetManager;

template <typename T>
concept BakerType = std::is_base_of<AssetBaker, T>::value;

class AssetPackage
{
	friend class EAssetManager;
public:
	
	AssetPackage(EAssetManager* assetManager, AssetRegistry* registry)
		: mAssetManager(assetManager)
		, mRegistry(registry)
	{

	}

	virtual ~AssetPackage() = default;
	
	template<typename T>
	bool ImportReference(const Gleam::Filesystem::Path& path, const T::ImportSettings& settings)
	{
		auto package = T(mAssetManager, mRegistry);
		if (package.Import(path, settings))
		{
			Gleam::ArrayUtils::Append<Gleam::RefCounted<AssetBaker>>(mBakers, package.mBakers);
			return true;
		}
		return false;
	}
	
	template<BakerType T, typename ... Args>
	Gleam::RefCounted<T> EmplaceBaker(Args&& ... args)
	{
		auto baker = CreateRef<T>(std::forward<Args>(args)...);
		mBakers.emplace_back(baker);
		mRegistry->RegisterAsset(baker->Filename(), baker->TypeGuid());
		return baker;
	}
	
	const AssetRegistry* Registry() const
	{
		return mRegistry;
	}
	
	const EAssetManager* AssetManager() const
	{
		return mAssetManager;
	}
	
private:
	
	AssetRegistry* mRegistry;
	EAssetManager* mAssetManager;
	Gleam::TArray<Gleam::RefCounted<AssetBaker>> mBakers;
};

#define AssetPackageType(type) type(EAssetManager* assetManager, AssetRegistry* registry) : AssetPackage(assetManager, registry) {}

} // namespace GEditor
