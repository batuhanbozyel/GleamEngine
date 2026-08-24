#include "AssetWriter.h"
#include "AssetRegistry.h"

#include "Assets/Asset.h"
#include "Assets/AssetHeader.h"

#include "IO/File.h"
#include "IO/Filesystem.h"

#include "Serialization/BinarySerializer.h"

using namespace GEditor;

void BinaryAssetWriter::Write(const Gleam::Path& directory, const AssetItem& item, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const
{
	auto filename = Gleam::TWString(item.reference.guid.ToString()) + Gleam::Asset::Extension();
	auto file = Gleam::Filesystem::Create(directory / filename, Gleam::FileType::Binary);
	auto& stream = file->GetStream();

	Gleam::AssetHeader header;
	header.typeGuid = item.type;
	header.name = item.name;

	if (not mBlobs.empty())
	{
		header.dataTable.blobs.resize(mBlobs.size());

		uint64_t blobOffset = 0;
		for (uint32_t i = 0; i < mBlobs.size(); ++i)
		{
			auto& blob = header.dataTable.blobs[i];
			blob.type = mBlobs[i].type;
			blob.slot = mBlobs[i].slot;
			blob.platform = mBlobs[i].platform;
			blob.backend = mBlobs[i].backend;
			blob.range.offset = blobOffset;
			blob.range.size = mBlobs[i].size;

			blobOffset += mBlobs[i].size;
		}
	}

	Gleam::BinarySerializer serializer;
	serializer.Serialize(header, stream);

	header.metadata = serializer.Serialize(metadata, classDesc, stream);
	header.bulkData.offset = header.metadata.offset + header.metadata.size;
	for (const auto& blob : mBlobs)
	{
		stream.write(reinterpret_cast<const char*>(blob.data), blob.size);
		header.bulkData.size += blob.size;
	}

	stream.seekp(0);
	serializer.Serialize(header, stream);
}

uint32_t BinaryAssetWriter::AddBlob(const Gleam::AssetBlobType& type,
									const void* data,
									uint64_t size,
									Gleam::AssetPlatform platform,
									Gleam::AssetBackend backend)
{
	uint32_t slot = mSlotCounts[type.guid]++;
	AddBlobVariant(type, slot, data, size, platform, backend);
	return slot;
}

void BinaryAssetWriter::AddBlobVariant(const Gleam::AssetBlobType& type,
									   uint32_t slot,
									   const void* data,
									   uint64_t size,
									   Gleam::AssetPlatform platform,
									   Gleam::AssetBackend backend)
{
	mBlobs.emplace_back(DataBlob{
		.data = data,
		.size = size,
		.type = type,
		.slot = slot,
		.platform = platform,
		.backend = backend
	});
}
