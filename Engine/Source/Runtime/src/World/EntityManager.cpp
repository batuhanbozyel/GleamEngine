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
	mRegistry.ctx().emplace<ChangeTracker>();
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
	GetChangeTracker().MarkChanged<Transform>(handle);
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

void* EntityManager::AddComponent(EntityHandle entity, uint32_t typeHash)
{
	auto func = entt::resolve(typeHash).func("AddComponent"_hs);
	GLEAM_ASSERT(func, "Component type is not registered with the scripting system: {}",
		Reflection::GetClass(typeHash) ? Reflection::GetClass(typeHash)->ResolveName() : std::string_view("unknown"));

	auto component = func.invoke({}, Ref<Entity>(GetComponent<Entity>(entity)));
	return const_cast<void*>(component.base().data());
}

void EntityManager::RemoveComponent(EntityHandle entity, uint32_t typeHash)
{
	auto func = entt::resolve(typeHash).func("RemoveComponent"_hs);
	GLEAM_ASSERT(func, "Component type is not registered with the scripting system: {}",
		Reflection::GetClass(typeHash) ? Reflection::GetClass(typeHash)->ResolveName() : std::string_view("unknown"));

	func.invoke({}, Ref<Entity>(GetComponent<Entity>(entity)));
}

void EntityManager::CopyFrom(const EntityManager& source)
{
	GLEAM_ASSERT(mHandles.empty(), "Entities can only be copied into an empty entity manager!");
	GLEAM_ASSERT(mSingletonEntity == source.mSingletonEntity, "Singleton entities do not match!");

	TArray<EntityHandle> handles;
	source.ForEach([&](EntityHandle handle)
	{
		if (source.IsEditorOnly(handle) == false)
		{
			handles.push_back(handle);
		}
	});

	for (auto handle : handles)
	{
		const auto created = mRegistry.create(handle);
		GLEAM_ASSERT(created == handle, "Copied entity handle does not match the source!");
	}

	const auto sourceEntities = source.FindStorage<Entity>();
	auto& entities = GetStorage<Entity>();
	auto& tracker = GetChangeTracker();
	for (auto handle : handles)
	{
		auto& entity = entities.emplace(handle, sourceEntities->get(handle));
		entity.mRegistry = &mRegistry;

		if (entity.mParent != InvalidEntity and mRegistry.valid(entity.mParent) == false)
		{
			entity.mParent = InvalidEntity;
		}

		auto it = eastl::remove_if(entity.mChildren.begin(), entity.mChildren.end(), [this](EntityHandle child)
		{
			return mRegistry.valid(child) == false;
		});
		entity.mChildren.erase(it, entity.mChildren.end());

		tracker.MarkChanged<Transform>(handle);
		mHandles.emplace(entity.GetGuid(), handle);
	}

	const auto entityStorageId = entt::type_hash<EntityHandle>::value();
	const auto entityClassId = Reflection::GetClass<Entity>().TypeHash();
	for (const auto& [id, storage] : source.mRegistry.storage())
	{
		if (id == entityStorageId or id == entityClassId)
		{
			continue;
		}

		const auto classDesc = Reflection::GetClass(id);
		if (classDesc and classDesc->HasAttribute<Reflection::Attribute::EditorOnly>())
		{
			continue;
		}

		auto copyStorage = entt::resolve(id).func("CopyStorage"_hs);
		GLEAM_ASSERT(copyStorage, "Component storage is not registered for copying: {}", classDesc ? classDesc->ResolveName() : std::string_view("unknown"));
		copyStorage.invoke({}, Ref<EntityManager>(*this), Ref<const EntityManager>(source));
	}
}

bool EntityManager::IsValid(EntityHandle entity) const
{
	return mRegistry.valid(entity);
}

bool EntityManager::IsEditorOnly(EntityHandle entity) const
{
	bool editorOnly = false;
	Visit(entity, [&](const void* component, const Reflection::ClassDescription& classDesc)
	{
		editorOnly = editorOnly or classDesc.HasAttribute<Reflection::Attribute::EditorOnly>();
	});
	return editorOnly;
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
