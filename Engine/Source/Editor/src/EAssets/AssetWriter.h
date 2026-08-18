#pragma once
#include "Core/GUID.h"
#include "IO/Path.h"
#include "Container/Array.h"

#include <Reflection/Reflection.h>
#include <Runtime.Reflection.generated.h>

namespace GEditor {

struct AssetItem;

class BinaryAssetWriter
{
public:

	uint32_t AddBlob(const void* data, uint64_t size);

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
	};
	Gleam::TArray<DataBlob> mBlobs;

};

} // namespace GEditor
