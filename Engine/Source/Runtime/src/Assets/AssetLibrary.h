#pragma once
#include "Filesystem.h"

namespace Gleam {

template <typename T>
concept AssetType = requires(T, const Filesystem::path& path)
{
	{ T::Import(path) -> T };
};

template<AssetType T>
class AssetLibrary final
{
public:

	static const AssetType& Import(const Filesystem::path& path)
	{
		auto assetCacheIt = mAssetCache.find(path);
		if (assetCacheIt != mAssetCache.end())
		{
			return assetCacheIt->second;
		}

		return mAssetCache.insert(mAssetCache.end(), { path, T::Import(path) })->second;
	}

private:

	static inline HashMap<Filesystem::path, T> mAssetCache;

};

} // namespace Gleam
