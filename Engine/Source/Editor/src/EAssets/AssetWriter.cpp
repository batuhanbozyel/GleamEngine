#include "AssetWriter.h"
#include "AssetRegistry.h"

#include "Assets/Asset.h"
#include "Assets/AssetHeader.h"

#include "IO/File.h"
#include "IO/Filesystem.h"

#include "Serialization/BinarySerializer.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/JSONSerializer.h"

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
									Gleam::EnumFlag<Gleam::AssetBackend> backend)
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
									   Gleam::EnumFlag<Gleam::AssetBackend> backend)
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

void JSONAssetWriter::Write(const Gleam::Path& file, const Gleam::AssetHeader& assetHeader, const void* metadata, const Gleam::Reflection::ClassDescription& classDesc) const
{
	auto header = assetHeader;
	header.dataTable.blobs.resize(mBlobs.size());
	for (uint32_t i = 0; i < mBlobs.size(); ++i)
	{
		auto& blob = header.dataTable.blobs[i];
		blob.type = mBlobs[i].type;
		blob.slot = mBlobs[i].slot;
		blob.platform = mBlobs[i].platform;
		blob.backend = mBlobs[i].backend;
		blob.range.offset = i;
	}

	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());

	Gleam::JSONSerializer serializer;
	serializer.Serialize(header, root);

	rapidjson::Value metadataObject(rapidjson::kObjectType);
	rapidjson::Node metadataNode(metadataObject, root.allocator);
	serializer.Serialize(metadata, classDesc, metadataNode);
	root.AddMember("Metadata", metadataObject);

	rapidjson::Value blobs(rapidjson::kArrayType);
	for (const auto& blob : mBlobs)
	{
		rapidjson::Value value(blob.data->object, root.allocator);
		blobs.PushBack(value, root.allocator);
	}
	root.AddMember("Blobs", blobs);

	auto stream = Gleam::Filesystem::Create(file, Gleam::FileType::Text);
	rapidjson::OStreamWrapper ss(stream->GetStream());
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	root.object.Accept(writer);
}

uint32_t JSONAssetWriter::AddBlob(const Gleam::AssetBlobType& type,
								  const rapidjson::Node& data,
								  Gleam::AssetPlatform platform,
								  Gleam::EnumFlag<Gleam::AssetBackend> backend)
{
	uint32_t slot = mSlotCounts[type.guid]++;
	AddBlobVariant(type, slot, data, platform, backend);
	return slot;
}

void JSONAssetWriter::AddBlobVariant(const Gleam::AssetBlobType& type,
									 uint32_t slot,
									 const rapidjson::Node& data,
									 Gleam::AssetPlatform platform,
									 Gleam::EnumFlag<Gleam::AssetBackend> backend)
{
	mBlobs.emplace_back(DataBlob{
		.data = &data,
		.type = type,
		.slot = slot,
		.platform = platform,
		.backend = backend
	});
}
