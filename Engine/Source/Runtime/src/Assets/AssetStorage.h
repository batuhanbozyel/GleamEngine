#pragma once
#include "AssetHeader.h"
#include "AssetReference.h"

#include "Core/Macro.h"
#include "IO/Path.h"
#include "IO/Filesystem.h"
#include "Container/Hash.h"
#include "Container/String.h"

#include <Reflection/Reflection.h>

#include <mutex>

namespace Gleam {

class Buffer;
class Texture;

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

struct ChunkDestinationTexture
{
	const Texture* resource = nullptr;
	uint32_t mip = 0;
	uint32_t slice = 0;
};

struct ChunkDestination
{
	enum class Kind : uint8_t
	{
		Memory,
		Buffer,
		Texture
	};
	Kind kind = Kind::Memory;

	union
	{
		ChunkDestinationMemory memory;
		ChunkDestinationBuffer buffer;
		ChunkDestinationTexture texture;
	};
};

struct AssetDataReadRequest
{
	AssetReference asset;
	BufferRange range;
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

NO_DISCARD FORCE_INLINE ChunkDestination MakeTextureDestination(const Texture& texture, uint32_t mip, uint32_t slice)
{
	return ChunkDestination{ .kind = ChunkDestination::Kind::Texture, .texture = { .resource = &texture, .mip = mip, .slice = slice } };
}

class AssetStorage final
{
public:

	AssetStorage(const Path& directory);

	bool Contains(const AssetReference& ref) const;

	AssetHeader ReadAsset(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* metadata) const;

	void Enqueue(const AssetDataReadRequest& request);

	AssetStreamFence Submit();

	void Wait(AssetStreamFence fence);

	void EmplaceAssetPath(const Path& path);

	void RemoveAssetPath(const Path& path);

	const Path& GetAssetPath(const AssetReference& ref) const;

private:

	void ReadData(FileStream& stream, const AssetDataReadRequest& request) const;

	Path mDirectory;

	std::mutex mMutex;

	HashMap<AssetReference, Path> mEntries;

	TArray<AssetDataReadRequest> mPendingRequests;

	uint64_t mFenceValue = 0;

};

} // namespace Gleam
