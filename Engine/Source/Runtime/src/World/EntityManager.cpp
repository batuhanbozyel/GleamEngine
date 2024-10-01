#include "gpch.h"
#include "EntityManager.h"
#include "World.h"

#include "Core/Globals.h"
#include "Core/Application.h"
#include "Assets/AssetManager.h"

using namespace Gleam;

Entity& EntityManager::CreateFromPrefab(const Prefab& prefab)
{
	auto assetManager = Globals::GameInstance->GetSubsystem<AssetManager>();
	const auto& asset = assetManager->GetAsset(prefab.reference);

	auto fullpath = Globals::ProjectContentDirectory / asset.path;
	auto file = Filesystem::Open(fullpath, FileType::Text);

	World world;
	world.Deserialize(file.GetStream());

	auto& entityManager = world.GetEntityManager();
	auto entityCount = entityManager.GetEntityCount();
	if (entityCount > 1)
	{
		auto& root = CreateEntity(prefab.reference.guid);
		entityManager.ForEach<Entity>([&](Entity& source)
		{
			auto& entity = CreateCopy(entityManager, source);
			entity.SetParent(root);
		});
		return root;
	}
	auto& entity = *entityManager.mRegistry.storage<Entity>().begin();
	auto& root = CreateCopy(entityManager, entity);
	return root;
}

Entity& EntityManager::CreateCopy(EntityManager& from, const Entity& source)
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
	auto handle = mRegistry.create();
	auto& entity = AddComponent<Entity>(handle, handle, &mRegistry, guid);
	mHandles[guid] = handle;
	return entity;
}

void EntityManager::DestroyEntity(EntityHandle entity)
{
	const auto& guid = mRegistry.get<Entity>(entity).GetGuid();
	mHandles.erase(guid);
	mRegistry.destroy(entity);
}

void EntityManager::DestroyEntity(const TArray<EntityHandle>& entities)
{
	for (auto entity : entities)
	{
		const auto& guid = mRegistry.get<Entity>(entity).GetGuid();
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
