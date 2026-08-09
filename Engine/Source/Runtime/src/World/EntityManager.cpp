#include "gpch.h"
#include "EntityManager.h"
#include "World.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"
#include "Serialization/JSONSerializer.h"

using namespace Gleam;

EntityManager::EntityManager()
{
	mSingletonEntity = mRegistry.create();
}

Entity& EntityManager::CreateFromPrefab(const AssetReference& ref)
{
	const auto& path = Globals::GameInstance->GetSubsystem<AssetManager>()->GetAssetPath(ref);
	auto file = Filesystem::OpenRead(Globals::ProjectContentDirectory / path, FileType::Text);

	Prefab prefab;
	auto root = prefab.Deserialize(*this, file->GetStream());
	return GetComponent<Entity>(root);
}

Entity& EntityManager::CreateEntity(const TString& name, const Guid& guid)
{
	auto it = mHandles.find(guid);
	if (it != mHandles.end())
	{
		return GetComponent<Entity>(it->second);
	}

	auto handle = mRegistry.create();
	auto& entity = AddComponent<Entity>(handle, handle, &mRegistry, name, guid);
	mHandles.emplace_hint(mHandles.end(), guid, handle);
	return entity;
}

void EntityManager::DestroyEntity(EntityHandle entity)
{
	auto& entityComponent = GetComponent<Entity>(entity);
	if (entityComponent.HasParent())
	{
		auto& parentEntity = entityComponent.GetParentEntity();
		auto it = eastl::remove(parentEntity.mChildren.begin(), parentEntity.mChildren.end(), entity);
		parentEntity.mChildren.erase(it);
	}
	DestroyHierarchy(entity);
}

void EntityManager::DestroyHierarchy(EntityHandle entity)
{
	const auto& entityComponent = GetComponent<Entity>(entity);
	auto children = entityComponent.GetChildren();
	auto guid = entityComponent.GetGuid();
	
	for (auto child : children)
	{
		DestroyHierarchy(child);
	}
	
	mHandles.erase(guid);
	mRegistry.destroy(entity);
}

void EntityManager::Visit(EntityHandle entity, VisitFn&& fn)
{
	for (const auto& [id, storage] : mRegistry.storage())
	{
		if (storage.contains(entity))
		{
			const auto classDesc = Reflection::GetClass(id);
			if (classDesc && classDesc->Guid() != Guid::InvalidGuid())
			{
				void* component = storage.value(entity);
				fn(component, *classDesc);
			}
		}
	}
}

void EntityManager::Visit(EntityHandle entity, ConstVisitFn&& fn) const
{
	for (const auto& [id, storage] : mRegistry.storage())
	{
		if (storage.contains(entity))
		{
			const auto classDesc = Reflection::GetClass(id);
			if (classDesc && classDesc->Guid() != Guid::InvalidGuid())
			{
				const void* component = storage.value(entity);
				fn(component, *classDesc);
			}
		}
	}
}

void* EntityManager::FindComponent(EntityHandle entity, uint32_t typeHash)
{
	auto storage = mRegistry.storage(typeHash);
	if (storage && storage->contains(entity))
	{
		return storage->value(entity);
	}
	return nullptr;
}

void* EntityManager::FindSingleton(uint32_t typeHash)
{
	return FindComponent(mSingletonEntity, typeHash);
}

bool EntityManager::IsValid(EntityHandle entity) const
{
	return mRegistry.valid(entity);
}

uint32_t EntityManager::GetEntityCount() const
{
	return static_cast<uint32_t>(mRegistry.storage<EntityHandle>()->size());
}

EntityHandle EntityManager::GetEntity(const EntityReference& ref) const
{
	auto it = mHandles.find(ref.guid);
	if (it != mHandles.end())
	{
		return it->second;
	}
	return InvalidEntity;
}
