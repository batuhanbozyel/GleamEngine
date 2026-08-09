#include "gpch.h"
#include "EntitySerializer.h"
#include "JSONInternal.h"
#include "JSONSerializer.h"

using namespace Gleam;

static void SerializeEntity(const EntityManager& entityManager, EntityHandle handle, rapidjson::Node& entitiesNode)
{
	const auto& entity = entityManager.GetComponent<Entity>(handle);
	auto entityGuid = entity.GetGuid().ToString();

	rapidjson::Value entityObject(rapidjson::kObjectType);
	rapidjson::Node entityNode(entityObject, entitiesNode.allocator);

	entityNode.AddMember("Entity", entityGuid);
	entityNode.AddMember("Active", entity.IsActive());
	entityNode.AddMember("Name", entity.GetName());

	if (entity.HasParent())
	{
		const auto& parent = entityManager.GetComponent<Entity>(entity.GetParent());
		auto parentGuid = parent.GetGuid().ToString();
		entityNode.AddMember("Parent", parentGuid);
	}

	rapidjson::Value transformObject(rapidjson::kObjectType);
	rapidjson::Node transformNode(transformObject, entitiesNode.allocator);
	{
		JSONSerializer serializer;
		serializer.Serialize<Transform>(entity.GetLocalTransform(), transformNode);
	}
	entityNode.AddMember("Transform", transformObject);

	rapidjson::Value componentsArrayObject(rapidjson::kArrayType);
	rapidjson::Node componentsArrayNode(componentsArrayObject, entitiesNode.allocator);
	entityManager.Visit(handle, [&](const void* component, const Reflection::ClassDescription& classDesc)
	{
		if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
		{
			rapidjson::Value componentObject(rapidjson::kObjectType);
			rapidjson::Node componentNode(componentObject, entitiesNode.allocator);

			JSONSerializer serializer;
			serializer.Serialize(component, classDesc, componentNode);

			componentsArrayNode.PushBack(componentObject);
		}
	});
	entityNode.AddMember("Components", componentsArrayObject);
	entitiesNode.PushBack(entityObject);
}

void EntitySerializer::SerializeEntities(const EntityManager& entityManager, TArrayView<const EntityHandle> entities, rapidjson::Node& root)
{
	rapidjson::Value entitiesObject(rapidjson::kArrayType);
	rapidjson::Node entitiesNode(entitiesObject, root.allocator);

	for (auto handle : entities)
	{
		SerializeEntity(entityManager, handle, entitiesNode);
	}
	root.AddMember("Entities", entitiesObject);
}

void EntitySerializer::Serialize(const EntityManager& entityManager, rapidjson::Node& root)
{
	TArray<EntityHandle> entities;
	entityManager.ForEach([&](EntityHandle handle)
	{
		entities.push_back(handle);
	});
	SerializeEntities(entityManager, entities, root);

	rapidjson::Value singletonComponents(rapidjson::kArrayType);
	rapidjson::Node singletonComponentsNode(singletonComponents, root.allocator);
	entityManager.VisitSingletons([&](const void* component, const Gleam::Reflection::ClassDescription& classDesc)
	{
		if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
		{
			rapidjson::Value componentObject(rapidjson::kObjectType);
			rapidjson::Node componentNode(componentObject, root.allocator);

			JSONSerializer serializer;
			serializer.Serialize(component, classDesc, componentNode);
			singletonComponentsNode.PushBack(componentObject);
		}
	});
	root.AddMember("Singletons", singletonComponents);
}

TArray<EntityHandle> EntitySerializer::DeserializeEntities(const rapidjson::ConstNode& root, EntityManager& entityManager)
{
	TArray<EntityHandle> entities;
	HashMap<Guid, TArray<Entity*>> entityParentRelation;
	for (const auto& entityObject : root["Entities"].GetArray())
	{
		auto entityGuid = TString(entityObject["Entity"].GetString());
		auto name = TString(entityObject["Name"].GetString());
		auto active = entityObject["Active"].GetBool();
		auto& entity = entityManager.CreateEntity(name, entityGuid);
		entity.SetActive(active);

		if (entityObject.HasMember("Parent"))
		{
			auto parentGuid = Guid(entityObject["Parent"].GetString());
			entityParentRelation[parentGuid].push_back(&entity);
		}

		const auto& transformObject = entityObject["Transform"];
		rapidjson::ConstNode transformNode(transformObject);
		{
			JSONSerializer serializer;
			auto transform = serializer.Deserialize<Transform>(transformNode);

			entity.SetTranslation(transform.position);
			entity.SetRotation(transform.rotation);
			entity.SetScale(transform.scale);
		}

		for (const auto& componentObject : entityObject["Components"].GetArray())
		{
			auto typeName = componentObject["TypeName"].GetString();
			const auto classDesc = Reflection::GetClass(typeName);

			auto meta = entt::resolve(classDesc->TypeHash());
			auto func = meta.func("AddComponent"_hs);
			auto component = func.invoke({}, Ref<Entity>(entity));
			GLEAM_ASSERT(component, "Entity component could not deserialize");

			JSONSerializer serializer;
			serializer.Deserialize(*classDesc, const_cast<void*>(component.base().data()), rapidjson::ConstNode(componentObject));
		}

		entities.emplace_back(entity);
	}

	for (auto& [parentGuid, entities] : entityParentRelation)
	{
		auto parent = entityManager.GetEntity(EntityReference{ .guid = parentGuid });
		for (auto& entity : entities)
		{
			entity->SetParent(parent);
		}
	}
	return entities;
}

TArray<EntityHandle> EntitySerializer::Deserialize(const rapidjson::ConstNode& root, EntityManager& entityManager)
{
	auto entities = DeserializeEntities(root, entityManager);

	for (const auto& singletonObject : root["Singletons"].GetArray())
	{
		auto typeName = singletonObject["TypeName"].GetString();
		const auto classDesc = Reflection::GetClass(typeName);

		auto meta = entt::resolve(classDesc->TypeHash());
		auto func = meta.func("SetSingleton"_hs);
		auto component = func.invoke({}, Ref<EntityManager>(entityManager));
		GLEAM_ASSERT(component, "Singleton component could not deserialize");

		JSONSerializer serializer;
		serializer.Deserialize(*classDesc, const_cast<void*>(component.base().data()), rapidjson::ConstNode(singletonObject));
	}

	return entities;
}
