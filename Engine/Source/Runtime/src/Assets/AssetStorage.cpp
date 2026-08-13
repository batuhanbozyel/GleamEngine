#include "gpch.h"
#include "AssetStorage.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "IO/Log.h"
#include "IO/File.h"

#include "Container/BinaryBuffer.h"

#include "Renderer/Buffer.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/CopyCommandBuffer.h"

#include "Serialization/BinarySerializer.h"

using namespace Gleam;

AssetStorage::AssetStorage(const Path& directory)
	: mDirectory(directory)
{

}

bool AssetStorage::Contains(const AssetReference& ref) const
{
	return mEntries.find(ref) != mEntries.end();
}

AssetStorage::AssetEntry& AssetStorage::LoadEntryHeader(const AssetReference& ref)
{
	auto it = mEntries.find(ref);
	if (it == mEntries.end())
	{
		GLEAM_CORE_ERROR("Asset could not located for GUID: {0}", ref.guid.ToString());
		GLEAM_ASSERT(false);
		static AssetEntry invalidEntry;
		return invalidEntry;
	}

	auto& entry = it->second;
	if (not entry.headerLoaded)
	{
		auto file = Filesystem::OpenRead(mDirectory / entry.path, FileType::Binary);
		auto& stream = file->GetStream();

		if (ReadAssetFileHeader(stream, entry.header))
		{
			entry.name.resize(static_cast<size_t>(entry.header.nameSize));
			stream.seekg(static_cast<std::streamoff>(entry.header.nameOffset));
			stream.read(entry.name.data(), static_cast<std::streamsize>(entry.header.nameSize));
			entry.headerLoaded = true;
		}
		else
		{
			GLEAM_CORE_ERROR("Asset is not a valid container: {0}", entry.path.String());
		}
	}
	return entry;
}

const AssetFileHeader& AssetStorage::ReadHeader(const AssetReference& ref)
{
	return LoadEntryHeader(ref).header;
}

const TString& AssetStorage::ReadName(const AssetReference& ref)
{
	return LoadEntryHeader(ref).name;
}

void AssetStorage::ReadMetadata(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* obj)
{
	const auto& entry = LoadEntryHeader(ref);
	if (entry.headerLoaded)
	{
		auto file = Filesystem::OpenRead(mDirectory / entry.path, FileType::Binary);
		auto& stream = file->GetStream();
		stream.seekg(static_cast<std::streamoff>(entry.header.metadataOffset));

		auto serializer = BinarySerializer();
		serializer.Deserialize(stream, classDesc, obj);
	}
}

const AssetChunkTable& AssetStorage::ReadChunkTable(const AssetReference& ref)
{
	auto& entry = LoadEntryHeader(ref);
	if (entry.headerLoaded and not entry.chunkTableLoaded)
	{
		if (entry.header.chunkCount > 0)
		{
			auto file = Filesystem::OpenRead(mDirectory / entry.path, FileType::Binary);
			auto& stream = file->GetStream();
			stream.seekg(static_cast<std::streamoff>(entry.header.chunkTableOffset));

			auto serializer = BinarySerializer();
			entry.chunkTable = serializer.Deserialize<AssetChunkTable>(stream);
		}
		entry.chunkTableLoaded = true;
	}
	return entry.chunkTable;
}

void AssetStorage::Enqueue(const ChunkReadRequest& request)
{
	mPendingRequests.push_back(request);
}

void AssetStorage::ReadChunk(FileStream& stream,
							 const AssetFileHeader& header,
							 const AssetChunkTable& chunkTable,
							 const ChunkReadRequest& request) const
{
	if (request.chunkIndex >= chunkTable.chunks.size())
	{
		GLEAM_CORE_ERROR("Chunk index {0} is out of range for GUID: {1}", request.chunkIndex, request.asset.guid.ToString());
		GLEAM_ASSERT(false);
		return;
	}

	const auto& chunk = chunkTable.chunks[request.chunkIndex];
	stream.seekg(static_cast<std::streamoff>(header.bulkDataOffset + chunk.offset));

	if (request.destination.kind == ChunkDestination::Kind::Memory)
	{
		GLEAM_ASSERT(request.destination.memory.size >= chunk.size, "Chunk read destination is too small.");
		stream.read(static_cast<char*>(request.destination.memory.buffer), static_cast<std::streamsize>(chunk.size));
	}
	else
	{
		BinaryBuffer staging(chunk.size);
		stream.read(static_cast<char*>(staging.data), static_cast<std::streamsize>(chunk.size));

		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(*request.destination.buffer.resource, staging.data, chunk.size, request.destination.buffer.offset);
	}
}

AssetStreamFence AssetStorage::Submit()
{
	while (not mPendingRequests.empty())
	{
		const auto asset = mPendingRequests.front().asset;
		const auto& entry = LoadEntryHeader(asset);
		const auto& chunkTable = ReadChunkTable(asset);

		auto file = Filesystem::OpenRead(mDirectory / entry.path, FileType::Binary);
		auto& stream = file->GetStream();

		for (auto it = mPendingRequests.begin(); it != mPendingRequests.end();)
		{
			if (it->asset == asset)
			{
				ReadChunk(stream, entry.header, chunkTable, *it);
				it = mPendingRequests.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	++mFenceValue;
	return AssetStreamFence{ .value = mFenceValue };
}

void AssetStorage::Wait(AssetStreamFence fence)
{

}

void AssetStorage::EmplaceAssetPath(const Path& path)
{
	Guid guid = TString(path.Stem());
	if (guid == Guid::InvalidGuid())
	{
		return;
	}

	auto relPath = path.IsRelative() ? path : Filesystem::Relative(path, mDirectory);

	std::lock_guard<std::mutex> lock(mMutex);
	auto& entry = mEntries[AssetReference{ .guid = guid }];
	entry.path = relPath;
	entry.headerLoaded = false;
	entry.chunkTableLoaded = false;
}

void AssetStorage::RemoveAssetPath(const Path& path)
{
	auto relPath = path.IsRelative() ? path : Filesystem::Relative(path, mDirectory);

	std::lock_guard<std::mutex> lock(mMutex);
	auto it = std::find_if(mEntries.begin(), mEntries.end(), [&](const auto& pair)
	{
		return pair.second.path == relPath;
	});

	if (it != mEntries.end())
	{
		mEntries.erase(it);
	}
}

const Path& AssetStorage::GetAssetPath(const AssetReference& ref) const
{
	auto it = mEntries.find(ref);
	if (it != mEntries.end())
	{
		return it->second.path;
	}

	GLEAM_ASSERT(false, "Asset could not located for GUID: {0}", ref.guid.ToString());
	static Path invalidPath;
	return invalidPath;
}
