//
//  EntitySnapshot.cpp
//  Editor
//

#include "EntitySnapshot.h"

#include "World/EntityManager.h"
#include "Serialization/JSONInternal.h"
#include "Serialization/EntitySerializer.h"

using namespace GEditor;

static void CollectSubtree(const Gleam::EntityManager& entityManager, Gleam::EntityHandle handle, Gleam::TArray<Gleam::EntityHandle>& entities)
{
	if (eastl::find(entities.begin(), entities.end(), handle) != entities.end())
	{
		return;
	}

	entities.push_back(handle);
	for (auto child : entityManager.GetComponent<Gleam::Entity>(handle).GetChildren())
	{
		CollectSubtree(entityManager, child, entities);
	}
}

void EntitySnapshot::Capture(const Gleam::EntityManager& entityManager, const Gleam::TArray<Gleam::EntityHandle>& entities)
{
	Gleam::TArray<Gleam::EntityHandle> handles;
	for (auto handle : entities)
	{
		CollectSubtree(entityManager, handle, handles);
	}

	mEntities.clear();
	mEntities.reserve(handles.size());
	for (auto handle : handles)
	{
		mEntities.push_back(entityManager.GetComponent<Gleam::Entity>(handle).GetGuid());
	}

	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Node root(document, document.GetAllocator());

	Gleam::EntitySerializer serializer;
	serializer.SerializeEntities(entityManager, handles, root);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer writer(buffer);
	writer.SetMaxDecimalPlaces(6);
	document.Accept(writer);
	mSerializedObject = buffer.GetString();
}

Gleam::TArray<Gleam::EntityHandle> EntitySnapshot::Restore(Gleam::EntityManager& entityManager) const
{
	rapidjson::Document document;
	document.Parse(mSerializedObject.c_str());

	Gleam::EntitySerializer serializer;
	return serializer.DeserializeEntities(rapidjson::ConstNode(document), entityManager);
}

Gleam::TArray<Gleam::EntityHandle> EntitySnapshot::Destroy(Gleam::EntityManager& entityManager) const
{
	auto handles = Resolve(entityManager);
	for (auto handle : handles)
	{
		// an entity may already be destroyed as a descendant of an earlier one
		if (entityManager.IsValid(handle))
		{
			entityManager.DestroyEntity(handle);
		}
	}
	return handles;
}

Gleam::TArray<Gleam::EntityHandle> EntitySnapshot::Resolve(const Gleam::EntityManager& entityManager) const
{
	Gleam::TArray<Gleam::EntityHandle> handles;
	handles.reserve(mEntities.size());
	for (const auto& guid : mEntities)
	{
		handles.push_back(entityManager.GetEntity(Gleam::EntityReference{ .guid = guid }));
	}
	return handles;
}
