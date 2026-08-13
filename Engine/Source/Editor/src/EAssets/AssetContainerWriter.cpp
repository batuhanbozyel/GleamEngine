#include "AssetContainerWriter.h"

#include "Assets/AssetContainer.h"

#include "IO/File.h"
#include "IO/Filesystem.h"

#include "Serialization/BinarySerializer.h"

using namespace GEditor;

AssetContainerWriter::AssetContainerWriter(const Gleam::Guid& typeGuid, const Gleam::TString& name)
	: mTypeGuid(typeGuid)
	, mName(name)
{

}

uint32_t AssetContainerWriter::AddChunk(const void* data, uint64_t size)
{
	mChunks.emplace_back(ChunkSource{ .data = data, .size = size });
	return static_cast<uint32_t>(mChunks.size() - 1u);
}

void AssetContainerWriter::Write(const Gleam::Path& path, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const
{
	auto file = Gleam::Filesystem::Create(path, Gleam::FileType::Binary);
	auto& stream = file->GetStream();

	auto header = Gleam::AssetFileHeader();
	header.typeGuid = mTypeGuid;
	header.chunkCount = static_cast<uint32_t>(mChunks.size());
	Gleam::WriteAssetFileHeader(stream, header);

	header.nameOffset = static_cast<uint64_t>(stream.tellp());
	header.nameSize = mName.size();
	stream.write(mName.data(), static_cast<std::streamsize>(mName.size()));

	auto serializer = Gleam::BinarySerializer();

	if (not mChunks.empty())
	{
		auto chunkTable = Gleam::AssetChunkTable();
		chunkTable.chunks.resize(mChunks.size());

		uint64_t chunkOffset = 0;
		for (uint32_t i = 0; i < mChunks.size(); ++i)
		{
			chunkTable.chunks[i].offset = chunkOffset;
			chunkTable.chunks[i].size = mChunks[i].size;
			chunkOffset += mChunks[i].size;
		}

		header.chunkTableOffset = static_cast<uint64_t>(stream.tellp());
		serializer.Serialize(chunkTable, stream);
		header.chunkTableSize = static_cast<uint64_t>(stream.tellp()) - header.chunkTableOffset;
	}

	header.metadataOffset = static_cast<uint64_t>(stream.tellp());
	serializer.Serialize(metadata, classDesc, stream);
	header.metadataSize = static_cast<uint64_t>(stream.tellp()) - header.metadataOffset;

	header.bulkDataOffset = static_cast<uint64_t>(stream.tellp());
	for (const auto& chunk : mChunks)
	{
		stream.write(static_cast<const char*>(chunk.data), static_cast<std::streamsize>(chunk.size));
	}

	stream.seekp(0);
	Gleam::WriteAssetFileHeader(stream, header);
}
