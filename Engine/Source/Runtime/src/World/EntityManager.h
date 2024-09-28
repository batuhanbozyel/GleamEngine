#pragma once
#include "Entity.h"

namespace Gleam {

class EntityManager final
{
	friend class Entity;
public:
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{})
    {
        mRegistry.view<ComponentTypes..., ExcludeComponents...>().each(fn);
    }
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{}) const
    {
        mRegistry.view<ComponentTypes..., ExcludeComponents...>().each(fn);
    }
    
    template<typename Func>
    void ForEach(Func&& fn)
    {
        for (auto [entt] : mRegistry.storage<EntityHandle>().each())
        {
            fn(entt);
        }
    }
    
    template<typename Func>
    void ForEach(Func&& fn) const
    {
        for (const auto [entt] : mRegistry.storage<EntityHandle>().each())
        {
            fn(entt);
        }
    }

	template<typename Func>
	void Visit(EntityHandle entity, Func&& fn)
	{
		for (const auto& [id, storage] : mRegistry.storage())
		{
			if (storage.contains(entity))
			{
				const auto& classDesc = Reflection::GetClass(id);
				if (classDesc.Guid() != Guid::InvalidGuid())
				{
					void* component = storage.value(entity);
					fn(component, classDesc);
				}
			}
		}
	}

	template<typename Func>
	void Visit(EntityHandle entity, Func&& fn) const
	{
		for (const auto& [id, storage] : mRegistry.storage())
		{
			if (storage.contains(entity))
			{
				const auto& classDesc = Reflection::GetClass(id);
				if (classDesc.Guid() != Guid::InvalidGuid())
				{
					const void* component = storage.value(entity);
					fn(component, classDesc);
				}
			}
		}
	}

	Entity& CreateEntity(const Guid& guid)
	{
		auto handle = mRegistry.create();
        auto& entity = AddComponent<Entity>(handle, handle, &mRegistry, guid);
		mHandles[guid] = handle;
        return entity;
	}

	template<typename ... Types>
	Entity& CreateEntity(const Guid& guid, Types&& ... components)
	{
        Entity& entity = CreateEntity(guid);
		(AddComponent<Types>(entity, components), ...);
		return entity;
	}

	void DestroyEntity(EntityHandle entity)
	{
		const auto& guid = mRegistry.get<Entity>(entity).GetGuid();
		mHandles.erase(guid);
		mRegistry.destroy(entity);
	}

	void DestroyEntity(const TArray<EntityHandle>& entities)
	{
		for (auto entity : entities)
		{
			const auto& guid = mRegistry.get<Entity>(entity).GetGuid();
			mHandles.erase(guid);
		}
		mRegistry.destroy(entities.begin(), entities.end());
	}
    
    template<typename T, typename ... Args>
    void SetSingletonComponent(Args&&... args)
    {
		const auto& desc = Reflection::GetClass<T>(); // hacky fix to register components to the database
        mRegistry.ctx().emplace<T>(std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& GetSingletonComponent()
    {
        return mRegistry.ctx().get<T>();
    }
    
    template<typename T>
    const T& GetSingletonComponent() const
    {
        return mRegistry.ctx().get<T>();
    }

	template<typename T, typename ... Args>
	T& AddComponent(EntityHandle entity, Args&&... args)
	{
		GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
		const auto& desc = Reflection::GetClass<T>(); // hacky fix to register components to the database
		return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
	}
    
    template<typename T, typename ... Args>
    void SetComponent(EntityHandle entity, Args&&... args)
    {
        GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
		const auto& desc = Reflection::GetClass<T>(); // hacky fix to register components to the database
        mRegistry.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
    }

	template<typename T>
	void RemoveComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		mRegistry.remove<T>(entity);
	}

	template<typename T>
	bool HasComponent(EntityHandle entity) const
	{
		return mRegistry.all_of<T>(entity);
	}

	template<typename T>
	T& GetComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		return mRegistry.get<T>(entity);
	}
    
    template<typename T>
    const T& GetComponent(EntityHandle entity) const
    {
        GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
        return mRegistry.get<T>(entity);
    }

	EntityHandle GetEntity(const EntityReference& ref) const
	{
		auto it = mHandles.find(ref.guid);
		if (it != mHandles.end())
		{
			return it->second;
		}
		return InvalidEntity;
	}

private:

	entt::registry mRegistry;

	HashMap<Guid, EntityHandle> mHandles;

};

} // namespace Gleam
