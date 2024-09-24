#include "gpch.h"
#include "World.h"
#include "Systems/TransformSystem.h"

using namespace Gleam;

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>

World::World(const TString& name)
	: mName(name)
{
	Time::Reset();
	AddSystem<TransformSystem>();
}

void World::Update()
{
	Time::Step();

	for (auto subsystem : mTickableSubsystems)
	{
		subsystem->Tick();
	}
	
	bool fixedUpdate = Time::fixedTime <= (Time::elapsedTime - Time::fixedDeltaTime);
	if (fixedUpdate)
	{
		Time::FixedStep();
		for (auto system : mSystems)
		{
			if (system->Enabled)
			{
				system->OnFixedUpdate(mEntityManager);
			}
		}
	}
	
	for (auto system : mSystems)
	{
		if (system->Enabled)
		{
			system->OnUpdate(mEntityManager);
		}
	}
}

void World::Serialize(FileStream& stream)
{
	rapidjson::Document root(rapidjson::kObjectType);
	root.AddMember("Name", rapidjson::StringRef(mName.c_str()), root.GetAllocator());

	rapidjson::Value entities(rapidjson::kArrayType);
	mEntityManager.ForEach([&](EntityHandle handle)
	{
		const auto& entity = mEntityManager.GetComponent<Entity>(handle);
		auto guid = entity.GetGuid().ToString();

		rapidjson::Value entityObject(rapidjson::kObjectType);
		entityObject.AddMember("Entity", rapidjson::StringRef(guid.c_str()), root.GetAllocator());
		entityObject.AddMember("Active", entity.IsActive(), root.GetAllocator());

		mEntityManager.Visit(handle, [&](const void* component, const Reflection::ClassDescription& classDesc)
		{
			// TODO: 
		});

		entities.PushBack(entityObject, root.GetAllocator());
	});
	root.AddMember("Entities", entities, root.GetAllocator());

	rapidjson::OStreamWrapper ss(stream);
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	root.Accept(writer);
}

void World::Deserialize(FileStream& stream)
{
	rapidjson::Document root(rapidjson::kObjectType);
	rapidjson::IStreamWrapper ss(stream);
	root.ParseStream(ss);

	mName = TString(root["Name"].GetString());
	for (const auto& entityObject : root["Entities"].GetArray())
	{
		auto guid = TString(entityObject["Entity"].GetString());
		auto active = entityObject["Active"].GetBool();
		auto& entity = mEntityManager.CreateEntity(guid);
		entity.SetActive(active);
	}
}