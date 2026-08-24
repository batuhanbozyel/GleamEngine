#pragma once
#include "Core/GUID.h"
#include "IO/Path.h"
#include "Container/Array.h"
#include "Container/Hash.h"
#include "Assets/AssetHeader.h"

#include <Reflection/Reflection.h>
#include <Runtime.Reflection.generated.h>

namespace GEditor {

struct AssetItem;

class BinaryAssetWriter
{
public:

	template<typename T>
	uint32_t AddBlob(const void* data,
					 uint64_t size,
					 Gleam::AssetPlatform platform,
					 Gleam::AssetBackend backend)
	{
		return AddBlob(Gleam::AssetUtils::BlobType<T>(), data, size, platform, backend);
	}

	template<typename T>
	void AddBlobVariant(uint32_t slot,
						const void* data,
						uint64_t size,
						Gleam::AssetPlatform platform,
						Gleam::AssetBackend backend)
	{
		AddBlobVariant(Gleam::AssetUtils::BlobType<T>(), slot, data, size, platform, backend);
	}

	uint32_t AddBlob(const Gleam::AssetBlobType& type,
					 const void* data,
					 uint64_t size,
					 Gleam::AssetPlatform platform,
					 Gleam::AssetBackend backend);

	void AddBlobVariant(const Gleam::AssetBlobType& type,
						uint32_t slot,
						const void* data,
						uint64_t size,
						Gleam::AssetPlatform platform,
						Gleam::AssetBackend backend);

	template<typename T>
	void Write(const Gleam::Path& directory, const AssetItem& item, const T& metadata) const
	{
		Write(directory, item, &metadata, Gleam::Reflection::GetClass<T>());
	}

	void Write(const Gleam::Path& directory, const AssetItem& item, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const;

private:

	struct DataBlob
	{
		const void* data = nullptr;
		uint64_t size = 0;
		Gleam::AssetBlobType type;
		uint32_t slot = 0;
		Gleam::AssetPlatform platform = Gleam::AssetPlatform::Common;
		Gleam::AssetBackend backend = Gleam::AssetBackend::Common;
	};
	Gleam::TArray<DataBlob> mBlobs;

	Gleam::HashMap<Gleam::Guid, uint32_t> mSlotCounts;

};

} // namespace GEditor
