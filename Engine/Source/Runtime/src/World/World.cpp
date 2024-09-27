#include "gpch.h"
#include "World.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/JSONSerializer.h"

using namespace Gleam;

World::World(const TString& name)
	: mName(name)
{
	Time::Reset();
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
	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());
	root.AddMember("Name", rapidjson::StringRef(mName.c_str()));

	rapidjson::Value entities(rapidjson::kArrayType);
	rapidjson::Node entitiesNode(entities, root.allocator);

	mEntityManager.ForEach([&](EntityHandle handle)
	{
		const auto& entity = mEntityManager.GetComponent<Entity>(handle);
		auto guid = entity.GetGuid().ToString();

		rapidjson::Value entityObject(rapidjson::kObjectType);
		rapidjson::Node entityNode(entityObject, root.allocator);

		entityNode.AddMember("Entity", rapidjson::StringRef(guid.c_str()));
		entityNode.AddMember("Active", entity.IsActive());

		mEntityManager.Visit(handle, [&](const void* component, const Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
			{
				JSONSerializer serializer(stream);
				serializer.Serialize(component, classDesc, entityNode);
			}
		});

		entitiesNode.PushBack(entityObject);
	});
	root.AddMember("Entities", entities);

	rapidjson::OStreamWrapper ss(stream);
	rapidjson::PrettyWriter writer(ss);
	writer.SetFormatOptions(rapidjson::PrettyFormatOptions::kFormatSingleLineArray);
	writer.SetMaxDecimalPlaces(6);
	writer.SetIndent('\t', 1);
	document.Accept(writer);
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