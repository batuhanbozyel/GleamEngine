#pragma once

namespace GEditor {

template<typename T>
class AssetImporter
{
public:

    static const T& Import(const Gleam::Filesystem::path& path)
	{
		return mAssets.push_back(T::Import(path));
	}

private:

	Gleam::TArray<T> mAssets;

};

} // namespace GEditor
