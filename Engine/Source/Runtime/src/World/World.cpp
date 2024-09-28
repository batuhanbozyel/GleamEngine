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
		auto entityGuid = entity.GetGuid().ToString();

		rapidjson::Value entityObject(rapidjson::kObjectType);
		rapidjson::Node entityNode(entityObject, root.allocator);

		entityNode.AddMember("Entity", entityGuid);
		entityNode.AddMember("Active", entity.IsActive());

		if (entity.HasParent())
		{
			const auto& parent = mEntityManager.GetComponent<Entity>(entity.GetParent());
			auto parentGuid = parent.GetGuid().ToString();
			entityNode.AddMember("Parent", parentGuid);
		}

		rapidjson::Value transformObject(rapidjson::kObjectType);
		rapidjson::Node transformNode(transformObject, root.allocator);
		{
			JSONSerializer serializer(stream);
			serializer.Serialize<Transform>(entity.GetLocalTransform(), transformNode);
		}
		entityNode.AddMember("Transform", transformObject);

		rapidjson::Value componentsArrayObject(rapidjson::kArrayType);
		rapidjson::Node componentsArrayNode(componentsArrayObject, root.allocator);
		mEntityManager.Visit(handle, [&](const void* component, const Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
			{
				rapidjson::Value componentObject(rapidjson::kObjectType);
				rapidjson::Node componentNode(componentObject, root.allocator);

				JSONSerializer serializer(stream);
				serializer.Serialize(component, classDesc, componentNode);

				componentsArrayNode.PushBack(componentObject);
			}
		});
		entityNode.AddMember("Components", componentsArrayObject);
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

	HashMap<Guid, TArray<Entity*>> entityParentRelation;
	for (const auto& entityObject : root["Entities"].GetArray())
	{
		auto entityGuid = TString(entityObject["Entity"].GetString());
		auto active = entityObject["Active"].GetBool();
		auto& entity = mEntityManager.CreateEntity(entityGuid);
		entity.SetActive(active);

		if (entityObject.HasMember("Parent"))
		{
			auto parentGuid = Guid(entityObject["Parent"].GetString());
			entityParentRelation[parentGuid].push_back(&entity);
		}

		const auto& transformObject = entityObject["Transform"];
		rapidjson::ConstNode transformNode(transformObject);
		{
			JSONSerializer serializer(stream);
			auto transform = serializer.Deserialize<Transform>(transformNode);

			entity.SetTranslation(transform.position);
			entity.SetRotation(transform.rotation);
			entity.SetScale(transform.scale);
		}

		for (const auto& componentObject : entityObject["Components"].GetArray())
		{
			auto typeName = componentObject["TypeName"].GetString();
			auto typeHash = Reflection::Database::GetTypeHash(typeName);
			const auto& classDesc = Reflection::GetClass(typeHash);

			auto meta = entt::resolve(static_cast<uint32_t>(typeHash));
			auto func = meta.func("AddComponent"_hs);
			auto component = func.invoke({}, Ref(entity));
			GLEAM_ASSERT(component, "Entity component could not deserialize");

			JSONSerializer serializer(stream);
			serializer.Deserialize(classDesc, component.data());
		}
	}

	for (auto& [parentGuid, entities] : entityParentRelation)
	{
		auto parent = mEntityManager.GetEntity(EntityReference{ .guid = parentGuid });
		for (auto& entity : entities)
		{
			entity->SetParent(parent);
		}
	}
}