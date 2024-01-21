#pragma once
#include "Gleam.h"

namespace GEditor {

template<typename T>
class AssetImporter
{
public:

    static const T& Import(const Gleam::Filesystem::path& path)
	{
		return mAssets.emplace_back(T::Import(path));
	}

private:

	static inline Gleam::TArray<T> mAssets;

};

} // namespace GEditor
