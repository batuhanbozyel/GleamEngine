#pragma once
#include "Entity.h"
#include "Prefab.h"
#include "EntityReference.h"

#include <functional>

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
		auto view = CreateView<ComponentTypes..., ExcludeComponents...>();
		view.each(fn);
    }
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{}) const
    {
		auto view = CreateView<ComponentTypes..., ExcludeComponents...>();
		view.each(fn);
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
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return mRegistry.storage<T>(classDesc.TypeHash()).emplace(entity, std::forward<Args>(args)...);
		}
		else
		{
			return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
		}
	}
    
    template<typename T, typename ... Args>
    void SetComponent(EntityHandle entity, Args&&... args)
    {
        GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return mRegistry.storage<T>(classDesc.TypeHash()).emplace_or_replace(entity, std::forward<Args>(args)...);
		}
		else
		{
			return mRegistry.emplace_or_replace<T>(entity, std::forward<Args>(args)...);
		}
    }

	template<typename T>
	void RemoveComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			mRegistry.storage<T>(classDesc.TypeHash()).remove(entity);
		}
		else
		{
			mRegistry.remove<T>(entity);
		}
	}

	template<typename T>
	bool HasComponent(EntityHandle entity) const
	{
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			if (const auto storage = mRegistry.storage<T>(classDesc.TypeHash()))
			{
				return storage->contains(entity);
			}
			return false;
		}
		else
		{
			return mRegistry.all_of<T>(entity);
		}
	}

	template<typename T>
	T& GetComponent(EntityHandle entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return mRegistry.storage<T>(classDesc.TypeHash()).get(entity);
		}
		else
		{
			return mRegistry.get<T>(entity);
		}
	}
    
    template<typename T>
    const T& GetComponent(EntityHandle entity) const
    {
        GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return mRegistry.storage<T>(classDesc.TypeHash())->get(entity);
		}
		else
		{
			return mRegistry.get<T>(entity);
		}
    }
	
	bool IsValid(EntityHandle entity) const;

	uint32_t GetEntityCount() const;

	EntityHandle GetEntity(const EntityReference& ref) const;

private:
	
	void DestroyHierarchy(EntityHandle entity);

	template<typename ... ComponentTypes, typename ... ExcludeComponents, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
	auto CreateView(Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{})
	{
		if constexpr (sizeof...(ExcludeComponents) == 0)
		{
			auto includeTuple = std::make_tuple(std::ref(GetStorage<ComponentTypes>())...);
			return entt::basic_view{ includeTuple };
		}
		else
		{
			auto includeTuple = std::make_tuple(std::ref(GetStorage<ComponentTypes>())...);
			auto excludeTuple = std::make_tuple(std::ref(GetStorage<ExcludeComponents>())...);
			return entt::basic_view{ includeTuple, excludeTuple };
		}
	}

	template<typename ... ComponentTypes, typename ... ExcludeComponents, typename = std::enable_if_t<sizeof...(ComponentTypes) + sizeof...(ExcludeComponents) != 0>>
	auto CreateView(Exclude<ExcludeComponents...> = Exclude<ExcludeComponents...>{}) const
	{
		if constexpr (sizeof...(ExcludeComponents) == 0)
		{
			auto includeTuple = std::make_tuple(std::ref(GetStorage<ComponentTypes>())...);
			return entt::basic_view{ includeTuple };
		}
		else
		{
			auto includeTuple = std::make_tuple(std::ref(GetStorage<ComponentTypes>())...);
			auto excludeTuple = std::make_tuple(std::ref(GetStorage<ExcludeComponents>())...);
			return entt::basic_view{ includeTuple, excludeTuple };
		}
	}

	template<typename T>
	auto& GetStorage()
	{
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return mRegistry.storage<T>(classDesc.TypeHash());
		}
		else
		{
			return mRegistry.storage<T>();
		}
	}

	template<typename T>
	const auto& GetStorage() const
	{
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			const auto& classDesc = Reflection::GetClass<T>();
			return *mRegistry.storage<T>(classDesc.TypeHash());
		}
		else
		{
			return *mRegistry.storage<T>();
		}
	}

	entt::registry mRegistry;
	EntityHandle mSingletonEntity;
	HashMap<Guid, EntityHandle> mHandles;
};

} // namespace Gleam
