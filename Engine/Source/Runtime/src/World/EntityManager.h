#pragma once
#include "Entity.h"
#include "Prefab.h"
#include "EntityReference.h"

#include <EASTL/algorithm.h>
#include <EASTL/array.h>

#include <functional>
#include <utility>

namespace Gleam {

struct AssetReference;

class EntityManager final
{
	friend class Entity;
	
	using VisitFn = std::function<void(void* component, const Reflection::ClassDescription& classDesc)>;
	using ConstVisitFn = std::function<void(const void* component, const Reflection::ClassDescription& classDesc)>;
public:

	EntityManager();

	Entity& CreateFromPrefab(const AssetReference& ref);

	Entity& CreateEntity(const TString& name, const Guid& guid);

	void DestroyEntity(EntityHandle entity);
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{})
    {
		Each<ComponentTypes...>(mRegistry, fn, Exclude<ExcludeComponents...>{});
    }
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{}) const
    {
		Each<ComponentTypes...>(mRegistry, fn, Exclude<ExcludeComponents...>{});
    }
    
    template<typename Func>
    void ForEach(Func&& fn)
    {
        for (auto [entity] : mRegistry.storage<EntityHandle>().each())
        {
			if (entity != mSingletonEntity)
			{
				fn(entity);
			}
        }
    }
    
    template<typename Func>
    void ForEach(Func&& fn) const
    {
        for (const auto [entity] : mRegistry.storage<EntityHandle>()->each())
        {
			if (entity != mSingletonEntity)
			{
				fn(entity);
			}
        }
    }

	void Visit(EntityHandle entity, VisitFn&& fn);

	void Visit(EntityHandle entity, ConstVisitFn&& fn) const;

	void* FindComponent(EntityHandle entity, uint32_t typeHash);

	void* FindSingleton(uint32_t typeHash);

	void* AddSingleton(uint32_t typeHash);

	void RemoveSingleton(uint32_t typeHash);

	void* AddComponent(EntityHandle entity, uint32_t typeHash);

	void RemoveComponent(EntityHandle entity, uint32_t typeHash);

	bool IsEditorOnly(EntityHandle entity) const;

	template<typename ... Types>
	Entity& CreateEntity(const TString& name, const Guid& guid, Types&& ... components)
	{
        Entity& entity = CreateEntity(name, guid);
		(AddComponent<Types>(entity, components), ...);
		return entity;
	}
    
    template<typename T, typename ... Args>
    T& SetSingleton(Args&&... args)
    {
		return AddComponent<T>(mSingletonEntity, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& GetSingleton()
    {
		return GetComponent<T>(mSingletonEntity);
    }
    
    template<typename T>
    const T& GetSingleton() const
    {
		return GetComponent<T>(mSingletonEntity);
    }

	template<typename T>
	void RemoveSingleton()
	{
		RemoveComponent<T>(mSingletonEntity);
	}

	void VisitSingletons(VisitFn&& fn)
	{
		Visit(mSingletonEntity, eastl::move(fn));
	}

	void VisitSingletons(ConstVisitFn&& fn) const
	{
		Visit(mSingletonEntity, eastl::move(fn));
	}

	template<typename T, typename ... Args>
	T& AddComponent(EntityHandle entity, Args&&... args)
	{
		GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
		if constexpr (IsTrackedComponent<T>::value)
		{
			GetChangeTracker().MarkAdded<T>(entity);
		}
		return AssureComponentStorage(mRegistry, ComponentTypeHash<T>()).template Emplace<T>(entity, std::forward<Args>(args)...);
	}
    
    template<typename T, typename ... Args>
    void SetComponent(EntityHandle entity, Args&&... args)
    {
		if constexpr (IsTrackedComponent<T>::value)
		{
			if (HasComponent<T>(entity))
			{
				GetChangeTracker().MarkChanged<T>(entity);
			}
			else
			{
				GetChangeTracker().MarkAdded<T>(entity);
			}
		}
		AssureComponentStorage(mRegistry, ComponentTypeHash<T>()).template EmplaceOrReplace<T>(entity, std::forward<Args>(args)...);
    }

	template<typename T>
	void RemoveComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		FindComponentStorage(mRegistry, ComponentTypeHash<T>())->remove(entity);
	}

	template<typename T>
	bool HasComponent(EntityHandle entity) const
	{
		if (const auto storage = FindComponentStorage(mRegistry, ComponentTypeHash<T>()))
		{
			return storage->contains(entity);
		}
		return false;
	}

	template<typename T>
	T& GetComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		if constexpr (IsTrackedComponent<T>::value)
		{
			GetChangeTracker().MarkChanged<T>(entity);
		}
		return FindComponentStorage(mRegistry, ComponentTypeHash<T>())->template Get<T>(entity);
	}
    
    template<typename T>
    const T& GetComponent(EntityHandle entity) const
    {
        GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		return FindComponentStorage(mRegistry, ComponentTypeHash<T>())->template Get<T>(entity);
    }
	
	template<typename T, typename Func>
	void ForEachChanged(Tick since, Func&& fn) const
	{
		const auto storage = FindComponentStorage(mRegistry, ComponentTypeHash<T>());
		if (storage == nullptr)
		{
			return;
		}

		const auto& tracker = GetChangeTracker();
		for (const auto entity : *storage)
		{
			if (tracker.IsChanged<T>(entity, since))
			{
				fn(entity, storage->template Get<T>(entity));
			}
		}
	}

	template<typename T, typename Func>
	void ForEachAdded(Tick since, Func&& fn) const
	{
		const auto storage = FindComponentStorage(mRegistry, ComponentTypeHash<T>());
		if (storage == nullptr)
		{
			return;
		}

		const auto& tracker = GetChangeTracker();
		for (const auto entity : *storage)
		{
			if (tracker.IsAdded<T>(entity, since))
			{
				fn(entity, storage->template Get<T>(entity));
			}
		}
	}

	template<typename T, auto Candidate, typename Instance>
	ComponentCallback OnComponentRemoved(Instance&& instance)
	{
		return ComponentCallback{ AssureComponentStorage(mRegistry, ComponentTypeHash<T>()).OnRemoved().template connect<Candidate>(std::forward<Instance>(instance)) };
	}

	ChangeTracker& GetChangeTracker()
	{
		return mRegistry.ctx().get<ChangeTracker>();
	}

	const ChangeTracker& GetChangeTracker() const
	{
		return mRegistry.ctx().get<const ChangeTracker>();
	}

	void CopyFrom(const EntityManager& source);

	bool IsValid(EntityHandle entity) const;

	uint32_t GetEntityCount() const;

	EntityHandle GetEntity(const EntityReference& ref) const;

private:
	
	void DestroyHierarchy(EntityHandle entity);

	template<typename ... ComponentTypes, typename Registry, typename Func, typename ... ExcludeComponents>
	static void Each(Registry& registry, Func& fn, Exclude<ExcludeComponents...>)
	{
		using StoragePtr = decltype(FindComponentStorage(registry, 0u));

		const eastl::array<StoragePtr, sizeof...(ComponentTypes)> includes = { FindComponentStorage(registry, ComponentTypeHash<ComponentTypes>())... };
		if (eastl::any_of(includes.begin(), includes.end(), [](StoragePtr storage) { return storage == nullptr; }))
		{
			return;
		}

		const eastl::array<StoragePtr, sizeof...(ExcludeComponents)> excludes = { FindComponentStorage(registry, ComponentTypeHash<ExcludeComponents>())... };

		auto leading = includes[0];
		for (const auto storage : includes)
		{
			if (storage->size() < leading->size())
			{
				leading = storage;
			}
		}

		// Sparse set iteration runs back to front, so removing the current entity's components inside fn is safe
		for (const auto entity : *leading)
		{
			const bool included = eastl::all_of(includes.begin(), includes.end(), [entity](StoragePtr storage) { return storage->contains(entity); });
			const bool excluded = eastl::any_of(excludes.begin(), excludes.end(), [entity](StoragePtr storage) { return storage != nullptr and storage->contains(entity); });
			if (included and excluded == false)
			{
				[&]<size_t ... I>(std::index_sequence<I...>)
				{
					if constexpr (std::is_invocable_v<Func&, EntityHandle, decltype(includes[I]->template Get<ComponentTypes>(entity))...>)
					{
						fn(entity, includes[I]->template Get<ComponentTypes>(entity)...);
					}
					else
					{
						fn(includes[I]->template Get<ComponentTypes>(entity)...);
					}
				}(std::index_sequence_for<ComponentTypes...>{});
			}
		}
	}

	entt::registry mRegistry;
	EntityHandle mSingletonEntity;
	HashMap<Guid, EntityHandle> mHandles;
};

} // namespace Gleam
