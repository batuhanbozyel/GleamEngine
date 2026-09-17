#pragma once
#include "AssetHeader.h"
#include "AssetReference.h"

#include "Core/Macro.h"
#include "IO/Path.h"
#include "IO/Filesystem.h"
#include "Container/Hash.h"
#include "Container/String.h"

#include "Renderer/StorageFile.h"

#include <Reflection/Reflection.h>

#include <mutex>

namespace Gleam {

class AssetStorage final
{
public:

	AssetStorage(const Path& directory);

	~AssetStorage();

	bool Contains(const AssetReference& ref) const;

	AssetHeader ReadAsset(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* metadata) const;

	void EmplaceAssetPath(const Path& path);

	void RemoveAssetPath(const Path& path);

	const Path& GetAssetPath(const AssetReference& ref) const;

	NO_DISCARD Path GetAssetFilePath(const AssetReference& ref) const;

	const StorageFile& GetAssetFile(const AssetReference& ref) const;

private:

	struct AssetEntry
	{
		Path path;
		StorageFile file;
	};

	Path mDirectory;

	mutable std::mutex mMutex;

	mutable HashMap<AssetReference, AssetEntry> mEntries;

};

} // namespace Gleam
