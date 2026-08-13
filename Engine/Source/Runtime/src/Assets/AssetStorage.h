#pragma once
#include "AssetContainer.h"
#include "AssetReference.h"

#include "Core/Macro.h"
#include "IO/Path.h"
#include "Container/Hash.h"
#include "Container/String.h"

#include <mutex>

namespace Gleam {

class Buffer;

struct AssetStreamFence
{
	uint64_t value = 0;
};

struct ChunkDestinationMemory
{
	void* buffer = nullptr;
	uint64_t size = 0;
};

struct ChunkDestinationBuffer
{
	const Buffer* resource = nullptr;
	uint64_t offset = 0;
};

struct ChunkDestination
{
	enum class Kind : uint8_t
	{
		Memory,
		Buffer
	};
	Kind kind = Kind::Memory;

	union
	{
		ChunkDestinationMemory memory;
		ChunkDestinationBuffer buffer;
	};
};

struct ChunkReadRequest
{
	AssetReference asset;
	uint32_t chunkIndex = 0;

	
	ChunkDestination destination;
};

NO_DISCARD FORCE_INLINE ChunkDestination MakeMemoryDestination(void* memory, uint64_t size)
{
	return ChunkDestination{ .kind = ChunkDestination::Kind::Memory, .memory = { .buffer = memory, .size = size } };
}

NO_DISCARD FORCE_INLINE ChunkDestination MakeBufferDestination(const Buffer& buffer, uint64_t offset)
{
	return ChunkDestination{ .kind = ChunkDestination::Kind::Buffer, .buffer = { .resource = &buffer, .offset = offset } };
}

class AssetStorage final
{
public:

	AssetStorage(const Path& directory);

	bool Contains(const AssetReference& ref) const;

	const AssetFileHeader& ReadHeader(const AssetReference& ref);

	const TString& ReadName(const AssetReference& ref);

	void ReadMetadata(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* obj);

	const AssetChunkTable& ReadChunkTable(const AssetReference& ref);

	void Enqueue(const ChunkReadRequest& request);

	AssetStreamFence Submit();

	void Wait(AssetStreamFence fence);

	void EmplaceAssetPath(const Path& path);

	void RemoveAssetPath(const Path& path);

	const Path& GetAssetPath(const AssetReference& ref) const;

private:

	struct AssetEntry
	{
		Path path;
		AssetFileHeader header;
		TString name;
		AssetChunkTable chunkTable;
		bool headerLoaded = false;
		bool chunkTableLoaded = false;
	};

	AssetEntry& LoadEntryHeader(const AssetReference& ref);

	void ReadChunk(FileStream& stream,
				   const AssetFileHeader& header,
				   const AssetChunkTable& chunkTable,
				   const ChunkReadRequest& request) const;

	Path mDirectory;

	std::mutex mMutex;

	HashMap<AssetReference, AssetEntry> mEntries;

	TArray<ChunkReadRequest> mPendingRequests;

	uint64_t mFenceValue = 0;

};

} // namespace Gleam
