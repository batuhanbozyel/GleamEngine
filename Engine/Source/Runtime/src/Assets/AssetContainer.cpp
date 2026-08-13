#include "gpch.h"
#include "AssetContainer.h"

using namespace Gleam;

bool Gleam::ReadAssetFileHeader(FileStream& stream, AssetFileHeader& header)
{
	stream.read(reinterpret_cast<char*>(&header.magic), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&header.version), sizeof(uint32_t));

	if (header.magic != AssetFileHeader::Magic or header.version != AssetFileHeader::Version)
	{
		return false;
	}

	stream.read(reinterpret_cast<char*>(&header.flags), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(&header.chunkCount), sizeof(uint32_t));
	stream.read(reinterpret_cast<char*>(header.typeGuid.mBytes), sizeof(header.typeGuid.mBytes));
	stream.read(reinterpret_cast<char*>(&header.nameOffset), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.nameSize), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.chunkTableOffset), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.chunkTableSize), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.metadataOffset), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.metadataSize), sizeof(uint64_t));
	stream.read(reinterpret_cast<char*>(&header.bulkDataOffset), sizeof(uint64_t));
	return true;
}

void Gleam::WriteAssetFileHeader(FileStream& stream, const AssetFileHeader& header)
{
	stream.write(reinterpret_cast<const char*>(&header.magic), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&header.version), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&header.flags), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(&header.chunkCount), sizeof(uint32_t));
	stream.write(reinterpret_cast<const char*>(header.typeGuid.mBytes), sizeof(header.typeGuid.mBytes));
	stream.write(reinterpret_cast<const char*>(&header.nameOffset), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.nameSize), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.chunkTableOffset), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.chunkTableSize), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.metadataOffset), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.metadataSize), sizeof(uint64_t));
	stream.write(reinterpret_cast<const char*>(&header.bulkDataOffset), sizeof(uint64_t));
}
