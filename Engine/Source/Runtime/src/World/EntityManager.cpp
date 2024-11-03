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
	const auto& path = assetManager->GetAssetPath(ref);
	auto prefab = assetManager->LoadDescriptor<Prefab>(ref);

	auto fullpath = Globals::ProjectContentDirectory / path;
	auto file = Filesystem::Open(fullpath, FileType::Text);

	if (prefab.entityCount > 1)
	{
		auto& root = CreateEntity(ref.guid);
		auto entities = prefab.Deserialize(*this, file.GetStream());
		for (auto handle : entities)
		{
			auto& entity = GetComponent<Entity>(handle);
			entity.SetParent(root);
		}
		return root;
	}

	auto root = prefab.Deserialize(*this, file.GetStream()).back();
	return GetComponent<Entity>(root);
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
