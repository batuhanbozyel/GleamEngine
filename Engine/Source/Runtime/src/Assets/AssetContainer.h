#pragma once
#include "Core/GUID.h"
#include "Container/Array.h"
#include "IO/Filesystem.h"

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

GSTRUCT(AssetChunk, "731EBF6E-1F3C-4EF8-9A5F-8247C7DF33D7", Serializable)
{
	GFIELD("57B29AEF-B944-41E4-B034-ACAFFABF0585", Serializable)
	uint64_t offset = 0;

	GFIELD("F3613654-D447-4CF3-8532-5114BB6E7A36", Serializable)
	uint64_t size = 0;
};

GSTRUCT(AssetChunkTable, "91170B46-96F7-48CE-9B16-9946705CF8C6", Serializable)
{
	GFIELD("27E3C612-F302-4E42-96D8-0447D272AFAD", Serializable)
	TArray<AssetChunk> chunks;
};

struct AssetFileHeader
{
	static constexpr uint32_t Magic = 0x54534147u;
	static constexpr uint32_t Version = 1u;

	uint32_t magic = Magic;
	uint32_t version = Version;
	uint32_t flags = 0;
	uint32_t chunkCount = 0;
	Guid typeGuid;
	uint64_t nameOffset = 0;
	uint64_t nameSize = 0;
	uint64_t chunkTableOffset = 0;
	uint64_t chunkTableSize = 0;
	uint64_t metadataOffset = 0;
	uint64_t metadataSize = 0;
	uint64_t bulkDataOffset = 0;
};

bool ReadAssetFileHeader(FileStream& stream, AssetFileHeader& header);

void WriteAssetFileHeader(FileStream& stream, const AssetFileHeader& header);

} // namespace Gleam
