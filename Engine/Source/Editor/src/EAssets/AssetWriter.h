#pragma once
#include "Core/GUID.h"
#include "IO/Path.h"
#include "Container/Array.h"
#include "Assets/AssetHeader.h"

#include <Reflection/Reflection.h>
#include <Runtime.Reflection.generated.h>

namespace GEditor {

struct AssetItem;

class BinaryAssetWriter
{
public:

	uint32_t AddBlob(const void* data,
					 uint64_t size,
					 uint64_t layoutHash,
					 Gleam::AssetPlatform platform,
					 Gleam::AssetBackend backend);

	void AddBlobVariant(uint32_t slot,
						const void* data,
						uint64_t size,
						uint64_t layoutHash,
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
		uint64_t layoutHash = 0;
		uint32_t slot = 0;
		Gleam::AssetPlatform platform = Gleam::AssetPlatform::Common;
		Gleam::AssetBackend backend = Gleam::AssetBackend::Common;
	};
	Gleam::TArray<DataBlob> mBlobs;

	uint32_t mSlotCount = 0;

};

} // namespace GEditor
