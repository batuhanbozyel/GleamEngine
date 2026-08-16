#pragma once
#include "Core/GUID.h"
#include "Container/Array.h"
#include "Container/BinaryBuffer.h"
#include "IO/Filesystem.h"

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

GSTRUCT(AssetDataTable, "91170B46-96F7-48CE-9B16-9946705CF8C6", Serializable)
{
	GFIELD("27E3C612-F302-4E42-96D8-0447D272AFAD", Serializable)
	TArray<BufferRange> blobs;
};

struct AssetFileHeader
{
	static constexpr uint32_t Magic = 0x54534147u;
	static constexpr uint32_t Version = 1u;

	uint32_t magic = Magic;
	uint32_t version = Version;
	uint32_t flags = 0;
	uint32_t blobCount = 0;
	Guid typeGuid;
	uint64_t nameOffset = 0;
	uint64_t nameSize = 0;
	uint64_t dataTableOffset = 0;
	uint64_t dataTableSize = 0;
	uint64_t metadataOffset = 0;
	uint64_t metadataSize = 0;
	uint64_t bulkDataOffset = 0;
};

bool ReadAssetFileHeader(FileStream& stream, AssetFileHeader& header);

void WriteAssetFileHeader(FileStream& stream, const AssetFileHeader& header);

} // namespace Gleam
