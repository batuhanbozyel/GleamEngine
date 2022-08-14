#pragma once
#include <filesystem>
#include "Renderer/Model.h"

namespace Gleam {

class AssetLibrary final
{
public:

	static std::filesystem::path GetDefaultAssetPath();

	static const Model& ImportModel(const std::filesystem::path& path);

private:

	static inline HashMap<std::filesystem::path, Model> sModelCache;

};

} // namespace Gleam