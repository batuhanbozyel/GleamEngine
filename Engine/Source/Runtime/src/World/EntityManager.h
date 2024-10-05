#pragma once
#include "Entity.h"
#include "Prefab.h"
#include "EntityReference.h"

namespace Gleam {

class EntityManager final
{
	friend class Entity;
	
	using VisitFn = std::function<void(void* component, const Reflection::ClassDescription& classDesc)>;
	using ConstVisitFn = std::function<void(const void* component, const Reflection::ClassDescription& classDesc)>;
public:

	Entity& CreateFromPrefab(const AssetReference& ref);

	Entity& CreateEntity(const Guid& guid);

	void DestroyEntity(EntityHandle entity);

	void DestroyEntity(const TArray<EntityHandle>& entities);
    
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
        for (const auto [entt] : mRegistry.storage<EntityHandle>()->each())
        {
            fn(entt);
        }
    }

	void Visit(EntityHandle entity, VisitFn&& fn)
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

	void Visit(EntityHandle entity, ConstVisitFn&& fn) const
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

	template<typename ... Types>
	Entity& CreateEntity(const Guid& guid, Types&& ... components)
	{
        Entity& entity = CreateEntity(guid);
		(AddComponent<Types>(entity, components), ...);
		return entity;
	}
    
    template<typename T, typename ... Args>
    void SetSingletonComponent(Args&&... args)
    {
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
		return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
	}
    
    template<typename T, typename ... Args>
    void SetComponent(EntityHandle entity, Args&&... args)
    {
        GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
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

	size_t GetEntityCount() const;

	EntityHandle GetEntity(const EntityReference& ref) const;

private:
	
	Entity& CreateCopy(EntityManager& from, const Entity& source);

	entt::registry mRegistry;

	HashMap<Guid, EntityHandle> mHandles;

};

} // namespace Gleam
