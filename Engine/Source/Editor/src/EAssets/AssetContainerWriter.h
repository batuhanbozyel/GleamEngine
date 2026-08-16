#pragma once
#include "Core/GUID.h"
#include "IO/Path.h"
#include "Container/Array.h"
#include "Container/String.h"

#include <Reflection/Reflection.h>
#include <Runtime.Reflection.generated.h>

namespace GEditor {

class AssetContainerWriter
{
public:

	AssetContainerWriter(const Gleam::Guid& typeGuid, const Gleam::TString& name);

	uint32_t AddBlob(const void* data, uint64_t size);

	template<typename T>
	void Write(const Gleam::Path& path, const T& metadata) const
	{
		Write(path, &metadata, Gleam::Reflection::GetClass<T>());
	}

	void Write(const Gleam::Path& path, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const;

private:

	struct ChunkSource
	{
		const void* data = nullptr;
		uint64_t size = 0;
	};

	Gleam::Guid mTypeGuid;

	Gleam::TString mName;

	Gleam::TArray<ChunkSource> mBlobs;

};

} // namespace GEditor
