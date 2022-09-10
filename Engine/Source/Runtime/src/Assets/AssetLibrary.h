#pragma once
#include <filesystem>
#include "Renderer/Model.h"

namespace Gleam {

class AssetLibrary final
{
public:

	static Filesystem::path GetDefaultAssetPath();

	static const Model& ImportModel(const Filesystem::path& path);

private:

	static inline HashMap<Filesystem::path, Model> sModelCache;

};

} // namespace Gleam
