//
//  EWorldManager.cpp
//  Editor
//

#include "EWorldManager.h"
#include "EAssets/AssetWriter.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "World/World.h"

#include "Serialization/JSONInternal.h"
#include "Serialization/EntitySerializer.h"

using namespace GEditor;

EWorldManager::EWorldManager(Gleam::World* world)
	: mEditWorld(world)
{

}

void EWorldManager::Save()
{
	const auto& path = Gleam::Globals::GameInstance->GetSubsystem<Gleam::AssetManager>()->GetAssetPath(mEditWorld->GetReference());
	SaveAs(Gleam::Globals::ProjectContentDirectory / path);
}

void EWorldManager::SaveAs(const Gleam::Path& file)
{
	rapidjson::Document entities(rapidjson::kObjectType);
	rapidjson::Node entitiesNode(entities, entities.GetAllocator());

	Gleam::EntitySerializer serializer;
	serializer.Serialize(mEditWorld->GetEntityManager(), entitiesNode);

	JSONAssetWriter writer;
	writer.AddBlob<Gleam::Entity>(entitiesNode, Gleam::AssetPlatform::Common, Gleam::AssetBackend::Common);
	writer.Write(file, mEditWorld->GetHeader(), mEditWorld->GetDescriptor());
}
