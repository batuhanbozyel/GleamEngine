#include "gpch.h"
#include "EntityManager.h"
#include "World.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

Entity& EntityManager::CreateFromPrefab(const AssetReference& ref)
{
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	const auto& asset = assetManager->GetAsset(ref);
	auto prefab = assetManager->Get<Prefab>(ref);

	auto fullpath = Globals::ProjectContentDirectory / asset.path;
	auto file = Filesystem::Open(fullpath, FileType::Text);

	if (prefab.entityCount > 1)
	{
		auto& root = CreateEntity(ref.guid);

		World world(prefab.name);
		auto& entityManager = world.GetEntityManager();
		prefab.Deserialize(entityManager, file.GetStream());
		entityManager.ForEach<Entity>([&](Entity& source)
		{
			auto& entity = CopyEntity(entityManager, source);
			entity.SetParent(root);
		});
		return root;
	}

	auto root = prefab.Deserialize(*this, file.GetStream()).back();
	return GetComponent<Entity>(root);
}

Entity& EntityManager::CopyEntity(EntityManager& from, const Entity& source)
{
	auto& copy = CreateEntity(source.GetGuid());
	copy.SetTranslation(source.GetLocalPosition());
	copy.SetRotation(source.GetLocalRotation());
	copy.SetScale(source.GetLocalScale());
	copy.SetParent(source.GetParent());
	copy.SetActive(source.IsActive());
	
	from.Visit(source, [&](const void* srcComponent, const Reflection::ClassDescription& classDesc)
	{
		if (classDesc.HasAttribute<Reflection::Attribute::EntityComponent>())
		{
			auto typeHash = Reflection::Database::GetTypeHash(classDesc.ResolveName());
			auto meta = entt::resolve(static_cast<uint32_t>(typeHash));
			auto func = meta.func("AddComponent"_hs);
			auto dstComponent = func.invoke({}, Ref<Entity>(copy));
			GLEAM_ASSERT(dstComponent, "Entity component could not copied");
			memcpy(dstComponent.data(), srcComponent, classDesc.GetSize());
		}
	});
	return copy;
}

Entity& EntityManager::CreateEntity(const Guid& guid)
{
	auto it = mHandles.find(guid);
	if (it != mHandles.end())
	{
		return GetComponent<Entity>(it->second);
	}

	auto handle = mRegistry.create();
	auto& entity = AddComponent<Entity>(handle, handle, &mRegistry, guid);
	mHandles.emplace_hint(mHandles.end(), guid, handle);
	return entity;
}

void EntityManager::DestroyEntity(EntityHandle entity)
{
	const auto& guid = GetComponent<Entity>(entity).GetGuid();
	mHandles.erase(guid);
	mRegistry.destroy(entity);
}

void EntityManager::DestroyEntity(const TArray<EntityHandle>& entities)
{
	for (auto entity : entities)
	{
		const auto& guid = GetComponent<Entity>(entity).GetGuid();
		mHandles.erase(guid);
	}
	mRegistry.destroy(entities.begin(), entities.end());
}

size_t EntityManager::GetEntityCount() const
{
	return mRegistry.storage<EntityHandle>()->size();
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
