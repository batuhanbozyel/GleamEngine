#include "AssetContainerWriter.h"

#include "Assets/AssetContainer.h"
#include "Assets/AssetHeader.h"

#include "IO/File.h"
#include "IO/Filesystem.h"

#include "Serialization/BinarySerializer.h"

using namespace GEditor;

AssetContainerWriter::AssetContainerWriter(const Gleam::Guid& typeGuid, const Gleam::TString& name)
	: mTypeGuid(typeGuid)
	, mName(name)
{

}

uint32_t AssetContainerWriter::AddBlob(const void* data, uint64_t size)
{
	mBlobs.emplace_back(ChunkSource{ .data = data, .size = size });
	return static_cast<uint32_t>(mBlobs.size() - 1u);
}

void AssetContainerWriter::Write(const Gleam::Path& path, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const
{
	auto file = Gleam::Filesystem::Create(path, Gleam::FileType::Binary);
	auto& stream = file->GetStream();

	Gleam::BinarySerializer serializer;

	auto header = Gleam::AssetHeader();
	header.typeGuid = mTypeGuid;
	header.blobCount = static_cast<uint32_t>(mBlobs.size());
	serializer.Serialize(header, stream);

	header.nameOffset = static_cast<uint64_t>(stream.tellp());
	header.nameSize = mName.size();
	stream.write(mName.data(), static_cast<std::streamsize>(mName.size()));

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

		header.dataTableOffset = static_cast<uint64_t>(stream.tellp());
		serializer.Serialize(dataTable, stream);
		header.dataTableSize = static_cast<uint64_t>(stream.tellp()) - header.dataTableOffset;
	}

	header.metadataOffset = static_cast<uint64_t>(stream.tellp());
	serializer.Serialize(metadata, classDesc, stream);
	header.metadataSize = static_cast<uint64_t>(stream.tellp()) - header.metadataOffset;

	header.bulkDataOffset = static_cast<uint64_t>(stream.tellp());
	for (const auto& blob : mBlobs)
	{
		stream.write(static_cast<const char*>(blob.data), static_cast<std::streamsize>(blob.size));
	}

	stream.seekp(0);
	serializer.Serialize(header, stream);
}
