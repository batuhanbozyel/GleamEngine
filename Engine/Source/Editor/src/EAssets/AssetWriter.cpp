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

	Gleam::BinarySerializer serializer;

	Gleam::AssetHeader header;
	header.typeGuid = item.type;
	header.blobCount = static_cast<uint32_t>(mBlobs.size());
	serializer.Serialize(header, stream);

	header.name = Gleam::BufferRange{ .offset = static_cast<uint64_t>(stream.tellp()), .size = item.name.size() };
	stream.write(item.name.data(), static_cast<std::streamsize>(item.name.size()));

	if (not mBlobs.empty())
	{
		auto dataTable = Gleam::AssetDataTable();
		dataTable.blobs.resize(mBlobs.size());

		uint64_t blobOffset = 0;
		for (uint32_t i = 0; i < mBlobs.size(); ++i)
		{
			dataTable.blobs[i].offset = blobOffset;
			dataTable.blobs[i].size = mBlobs[i].size;
			blobOffset += mBlobs[i].size;
		}
		header.dataTable = serializer.Serialize(dataTable, stream);
	}
	header.metadata = serializer.Serialize(metadata, classDesc, stream);

	header.bulkData = Gleam::BufferRange{ .offset = static_cast<uint64_t>(stream.tellp()), .size = 0 };
	for (const auto& blob : mBlobs)
	{
		stream.write(static_cast<const char*>(blob.data), static_cast<std::streamsize>(blob.size));
		header.bulkData.size += blob.size;
	}

	stream.seekp(0);
	serializer.Serialize(header, stream);
}

uint32_t BinaryAssetWriter::AddBlob(const void* data, uint64_t size)
{
	mBlobs.emplace_back(DataBlob{ .data = data, .size = size });
	return static_cast<uint32_t>(mBlobs.size() - 1u);
}
