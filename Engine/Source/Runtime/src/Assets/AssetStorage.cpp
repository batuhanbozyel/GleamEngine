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

static const AssetBlobDescriptor* ResolveBlob(const AssetDataTable& dataTable, const AssetBlobType& blobType, uint32_t slot, AssetPlatform platform, AssetBackend backend)
{
	for (const auto& blob : dataTable.blobs)
	{
		if (blob.platform != AssetPlatform::Common && blob.platform != platform)
		{
			continue;
		}

		if (blob.backend != AssetBackend::Common && blob.backend != backend)
		{
			continue;
		}

		if (blob.type.guid != blobType.guid || blob.type.version != blobType.version || blob.slot != slot)
		{
			continue;
		}

		return &blob;
	}
	return nullptr;
}

AssetStorage::AssetStorage(const Path& directory)
	: mDirectory(directory)
{

}

bool AssetStorage::Contains(const AssetReference& ref) const
{
	return mEntries.find(ref) != mEntries.end();
}

AssetHeader AssetStorage::ReadAsset(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* metadata) const
{
	auto file = Filesystem::OpenRead(mDirectory / GetAssetPath(ref), FileType::Binary);
	auto& stream = file->GetStream();

	auto serializer = BinarySerializer();
	auto header = serializer.Deserialize<AssetHeader>(stream);

	stream.seekg(static_cast<std::streamoff>(header.metadata.offset));
	serializer.Deserialize(stream, classDesc, metadata);
	return header;
}

const AssetBlobDescriptor* AssetStorage::FindBlob(const AssetHeader& header, const AssetBlobType& blobType, uint32_t slot, AssetBackend backend) const
{
	const auto blob = ResolveBlob(header.dataTable, blobType, slot, AssetUtils::Platform(), backend);
	if (blob == nullptr)
	{
		GLEAM_ASSERT(false, "Asset data blob {0}[{1}] has no variant for this target, Asset: {2} Version: {3}", blobType.guid.ToString(), slot, header.name, blobType.version);
	}
	return blob;
}

void AssetStorage::Enqueue(const AssetDataReadRequest& request)
{
	mPendingRequests.push_back(request);
}

void AssetStorage::ReadData(FileStream& stream, const AssetDataReadRequest& request) const
{
	stream.seekg(static_cast<std::streamoff>(request.range.offset));

	if (request.destination.kind == ChunkDestination::Kind::Memory)
	{
		GLEAM_ASSERT(request.destination.memory.size >= request.range.size, "Data read destination is too small.");
		stream.read(static_cast<char*>(request.destination.memory.buffer), static_cast<std::streamsize>(request.range.size));
	}
	else
	{
		BinaryBuffer staging(request.range.size);
		stream.read(static_cast<char*>(staging.data), static_cast<std::streamsize>(request.range.size));

		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		auto cmd = renderSystem->GetCopyCommandBuffer();
		cmd->Commit(*request.destination.buffer.resource, staging.data, request.range.size, request.destination.buffer.offset);
	}
}

AssetStreamFence AssetStorage::Submit()
{
	while (not mPendingRequests.empty())
	{
		const auto asset = mPendingRequests.front().asset;

		auto file = Filesystem::OpenRead(mDirectory / GetAssetPath(asset), FileType::Binary);
		auto& stream = file->GetStream();

		for (auto it = mPendingRequests.begin(); it != mPendingRequests.end();)
		{
			if (it->asset == asset)
			{
				ReadData(stream, *it);
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
	mEntries[AssetReference{ .guid = guid }] = relPath;
}

void AssetStorage::RemoveAssetPath(const Path& path)
{
	auto relPath = path.IsRelative() ? path : Filesystem::Relative(path, mDirectory);

	std::lock_guard<std::mutex> lock(mMutex);
	auto it = std::find_if(mEntries.begin(), mEntries.end(), [&](const auto& pair)
	{
		return pair.second == relPath;
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
		return it->second;
	}

	GLEAM_ASSERT(false, "Asset could not located for GUID: {0}", ref.guid.ToString());
	static Path invalidPath;
	return invalidPath;
}
