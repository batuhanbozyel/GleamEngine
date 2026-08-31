#include "gpch.h"
#include "AssetStorage.h"

#include "Core/Engine.h"
#include "Core/Globals.h"

#include "IO/Log.h"
#include "IO/File.h"

#include "Renderer/RenderSystem.h"
#include "Renderer/CopyCommandBuffer.h"

#include "Serialization/BinarySerializer.h"

using namespace Gleam;

AssetStorage::AssetStorage(const Path& directory)
	: mDirectory(directory)
{

}

AssetStorage::~AssetStorage()
{
	static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
	auto cmd = renderSystem->GetCopyCommandBuffer();

	for (auto& entry : mEntries)
	{
		cmd->CloseFile(entry.second.file);
	}
	mEntries.clear();
}

bool AssetStorage::Contains(const AssetReference& ref) const
{
	return mEntries.find(ref) != mEntries.end();
}

AssetHeader AssetStorage::ReadAsset(const AssetReference& ref, const Reflection::ClassDescription& classDesc, void* metadata) const
{
	auto file = Filesystem::OpenRead(GetAssetFilePath(ref), FileType::Binary);
	auto& stream = file->GetStream();

	auto serializer = BinarySerializer();
	auto header = serializer.Deserialize<AssetHeader>(stream);

	stream.seekg(header.metadata.offset);
	serializer.Deserialize(stream, classDesc, metadata);
	return header;
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

	if (entry.file.IsValid())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		renderSystem->GetCopyCommandBuffer()->CloseFile(entry.file);
	}
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
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		renderSystem->GetCopyCommandBuffer()->CloseFile(it->second.file);
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

	GLEAM_ASSERT(false, "Asset could not located for GUID: {}", ref.guid.ToString());
	static Path invalidPath;
	return invalidPath;
}

Path AssetStorage::GetAssetFilePath(const AssetReference& ref) const
{
	return mDirectory / GetAssetPath(ref);
}

const StorageFile& AssetStorage::GetAssetFile(const AssetReference& ref) const
{
	std::lock_guard<std::mutex> lock(mMutex);

	auto it = mEntries.find(ref);
	if (it == mEntries.end())
	{
		GLEAM_ASSERT(false, "Asset could not located for GUID: {}", ref.guid.ToString());
		static StorageFile invalidFile;
		return invalidFile;
	}

	auto& entry = it->second;
	if (not entry.file.IsValid())
	{
		static auto renderSystem = Globals::Engine->GetSubsystem<RenderSystem>();
		entry.file = renderSystem->GetCopyCommandBuffer()->OpenFile(mDirectory / entry.path);
	}
	return entry.file;
}
