#include "gpch.h"
#include "AssetLibrary.h"
#include "ModelImporter.h"

using namespace Gleam;

std::filesystem::path AssetLibrary::GetDefaultAssetPath()
{
	return std::filesystem::current_path().append("Assets/");
}

const Model& AssetLibrary::ImportModel(const std::filesystem::path& path)
{
	auto modelCacheIt = sModelCache.find(path);
	if (modelCacheIt != sModelCache.end())
	{
		return modelCacheIt->second;
	}

	return sModelCache.insert(sModelCache.end(), { path, ModelImporter::Import(path) })->second;
}
