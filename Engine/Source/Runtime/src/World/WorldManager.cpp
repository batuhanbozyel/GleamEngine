#include "gpch.h"
#include "WorldManager.h"
#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Application.h"
#include "IO/FileWatcher.h"
#include "Assets/AssetManager.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/JSONSerializer.h"
#include "Serialization/EntitySerializer.h"

using namespace Gleam;

void WorldManager::Initialize(Application* app)
{
	
}

void WorldManager::Shutdown(Application* app)
{
	mActiveWorld = nullptr;
	mLoadedWorlds.clear();
	mWorldsInBuild.clear();
}

void WorldManager::Configure(const WorldConfig& config)
{
	mWorldsInBuild = config.worlds;
}

void WorldManager::OpenWorld(uint32_t buildIndex)
{
	LoadWorld(buildIndex);
	SetActiveWorld(mLoadedWorlds[mWorldsInBuild[buildIndex]].get());
}

void WorldManager::OpenWorld(const Path& file)
{
	const AssetReference worldRef{ .guid = Guid(TString(file.Stem())) };
	LoadWorld(worldRef, file);
	SetActiveWorld(mLoadedWorlds[worldRef].get());
}

void WorldManager::LoadWorld(uint32_t buildIndex)
{
	const auto& worldRef = mWorldsInBuild[buildIndex];
	const auto& worldPath = Globals::GameInstance->GetSubsystem<AssetManager>()->GetAssetPath(worldRef);
	LoadWorld(worldRef, Globals::ProjectContentDirectory / worldPath);
}

void WorldManager::LoadWorld(const AssetReference& worldRef, const Path& worldFile)
{
	auto file = Filesystem::OpenRead(worldFile, FileType::Text);

	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::IStreamWrapper ss(file->GetStream());
	document.ParseStream(ss);

	JSONSerializer jsonSerializer;
	const auto header = jsonSerializer.Deserialize<AssetHeader>(rapidjson::ConstNode(document));
	const auto descriptor = jsonSerializer.Deserialize<WorldDescriptor>(rapidjson::ConstNode(document["Metadata"]));

	auto world = CreateScope<World>(worldRef, header, descriptor);

	const auto entities = world->FindBlob<Entity>(0, AssetPlatform::Common, AssetBackend::Common);
	const auto& blobs = document["Blobs"];

	EntitySerializer serializer;
	serializer.Deserialize(rapidjson::ConstNode(blobs[static_cast<rapidjson::SizeType>(entities->range.offset)]), world->GetEntityManager());
	mLoadedWorlds.emplace(worldRef, std::move(world));
}

void WorldManager::SetActiveWorld(World* world)
{
	GLEAM_ASSERT(world, "Active world cannot be null!");
	mActiveWorld = world;
}

World* WorldManager::GetActiveWorld() const
{
	return mActiveWorld;
}
