#pragma once

namespace Gleam {

class AssetManager final
{
public:

	AssetManager() = default;
	AssetManager(const Filesystem::path& directory);

private:

	Filesystem::path mRootDirectory;

};

} // namespace Gleam
