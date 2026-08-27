#include "gpch.h"
#include "Asset.h"

using namespace Gleam;

Asset::Asset(const AssetReference& reference, const AssetHeader& header)
	: mReference(reference)
	, mHeader(header)
{
	BuildBlobTable();
}

uint32_t Asset::ReferenceCount() const
{
	return mRefCount;
}

const AssetReference& Asset::GetReference() const
{
	return mReference;
}

const AssetHeader& Asset::GetHeader() const
{
	return mHeader;
}

const TString& Asset::GetName() const
{
	return mHeader.name;
}

const AssetBlobDescriptor* Asset::FindBlob(const AssetBlobType& type, uint32_t slot, AssetPlatform platform, AssetBackend backend) const
{
	const auto blob = ResolveBlob(type, slot, platform, backend);
	GLEAM_ASSERT(blob != nullptr, "Asset data blob {0}[{1}] has no variant for this target, Asset: {2} Version: {3}", type.guid.ToString(), slot, mHeader.name, type.version);
	return blob;
}

BufferRange Asset::GetBlobRange(const AssetBlobDescriptor& blob) const
{
	return BufferRange{ .offset = mHeader.bulkData.offset + blob.range.offset, .size = blob.range.size };
}

void Asset::BuildBlobTable()
{
	const auto& blobs = mHeader.dataTable.blobs;
	for (uint32_t i = 0; i < blobs.size(); ++i)
	{
		const auto& blob = blobs[i];

		auto& slots = mBlobTable[blob.type.guid];
		if (blob.slot >= slots.size())
		{
			slots.resize(blob.slot + 1);
		}
		slots[blob.slot].push_back(i);
	}
}

const AssetBlobDescriptor* Asset::ResolveBlob(const AssetBlobType& type, uint32_t slot, AssetPlatform platform, AssetBackend backend) const
{
	auto it = mBlobTable.find(type.guid);
	if (it == mBlobTable.end() || slot >= it->second.size())
	{
		return nullptr;
	}

	for (auto index : it->second[slot])
	{
		const auto& blob = mHeader.dataTable.blobs[index];
		if (blob.type.version == type.version && blob.platform == platform && blob.backend == backend)
		{
			return &blob;
		}
	}
	return nullptr;
}
