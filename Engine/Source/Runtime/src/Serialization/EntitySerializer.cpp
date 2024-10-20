#include "gpch.h"
#include "EntitySerializer.h"
#include "JSONInternal.h"
#include "JSONSerializer.h"

using namespace Gleam;

void EntitySerializer::Serialize(const EntityManager& entityManager, rapidjson::Node& root)
{
	rapidjson::Value entities(rapidjson::kArrayType);
	rapidjson::Node entitiesNode(entities, root.allocator);

	entityManager.ForEach([&](EntityHandle handle)
	{
		const auto& entity = entityManager.GetComponent<Entity>(handle);
		auto entityGuid = entity.GetGuid().ToString();

		rapidjson::Value entityObject(rapidjson::kObjectType);
		rapidjson::Node entityNode(entityObject, root.allocator);

		entityNode.AddMember("Entity", entityGuid);
		entityNode.AddMember("Active", entity.IsActive());

		if (entity.HasParent())
		{
			const auto& parent = entityManager.GetComponent<Entity>(entity.GetParent());
			auto parentGuid = parent.GetGuid().ToString();
			entityNode.AddMember("Parent", parentGuid);
		}

		rapidjson::Value transformObject(rapidjson::kObjectType);
		rapidjson::Node transformNode(transformObject, root.allocator);
		{
			JSONSerializer serializer;
			serializer.Serialize<Transform>(entity.GetLocalTransform(), transformNode);
		}
		entityNode.AddMember("Transform", transformObject);

		rapidjson::Value componentsArrayObject(rapidjson::kArrayType);
		rapidjson::Node componentsArrayNode(componentsArrayObject, root.allocator);
		entityManager.Visit(handle, [&](const void* component, const Reflection::ClassDescription& classDesc)
		{
			if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
			{
				rapidjson::Value componentObject(rapidjson::kObjectType);
				rapidjson::Node componentNode(componentObject, root.allocator);

				JSONSerializer serializer;
				serializer.Serialize(component, classDesc, componentNode);

				componentsArrayNode.PushBack(componentObject);
			}
		});
		entityNode.AddMember("Components", componentsArrayObject);
		entitiesNode.PushBack(entityObject);
	});
	root.AddMember("Entities", entities);
}

TArray<EntityHandle> EntitySerializer::Deserialize(const rapidjson::ConstNode& root, EntityManager& entityManager)
{
	TArray<EntityHandle> entities;
	HashMap<Guid, TArray<Entity*>> entityParentRelation;
	for (const auto& entityObject : root["Entities"].GetArray())
	{
		auto entityGuid = TString(entityObject["Entity"].GetString());
		auto active = entityObject["Active"].GetBool();
		auto& entity = entityManager.CreateEntity(entityGuid);
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
			auto typeHash = Reflection::Database::GetTypeHash(typeName);
			const auto& classDesc = Reflection::GetClass(typeHash);

			auto meta = entt::resolve(static_cast<uint32_t>(typeHash));
			auto func = meta.func("AddComponent"_hs);
			auto component = func.invoke({}, Ref<Entity>(entity));
			GLEAM_ASSERT(component, "Entity component could not deserialize");

			JSONSerializer serializer;
			serializer.Deserialize(classDesc, component.data(), rapidjson::ConstNode(componentObject));
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
