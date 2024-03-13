#pragma once
#include "IO/Filesystem.h"

namespace Gleam {

template<typename T>
class AssetLibrary final
{
public:

	static const RefCounted<T>& Import(const Filesystem::path& path)
	{
		auto assetCacheIt = mAssetCache.find(path);
		if (assetCacheIt != mAssetCache.end())
		{
			return assetCacheIt->second;
		}

		return mAssetCache.insert(mAssetCache.end(), { path, T::Import(path) })->second;
	}

private:

	static inline HashMap<Filesystem::path, RefCounted<T>> mAssetCache;

};

} // namespace Gleam
