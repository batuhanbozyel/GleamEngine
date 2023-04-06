#pragma once
#include "Entity.h"

namespace Gleam {

class EntityManager final
{    
public:
    
    template<typename ... ComponentTypes, typename ... ExcludeComponents, typename Func>
    void ForEach(Func&& fn, Exclude<ExcludeComponents...> = {})
    {
        mRegistry.view<ComponentTypes..., ExcludeComponents...>().each(fn);
    }

	Entity CreateEntity()
	{
		auto entity = mRegistry.create();
        AddComponent<Transform>(entity);
        return entity;
	}

	template<typename ... Types>
	Entity CreateEntity(Types&& ... components)
	{
        Entity entity = CreateEntity();
		(AddComponent<Types>(entity, components), ...);
		return entity;
	}

	void DestroyEntity(Entity entity)
	{
		mRegistry.destroy(entity);
	}

	void DestroyEntity(const TArray<Entity>& entities)
	{
		mRegistry.destroy(entities.begin(), entities.end());
	}

	template<typename T, typename ... Args>
	T& AddComponent(Entity entity, Args&&... args)
	{
		GLEAM_ASSERT(!HasComponent<T>(entity), "Entity already has the component!");
		return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
	}

	template<typename T>
	void RemoveComponent(Entity entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		mRegistry.remove<T>(entity);
	}

	template<typename T>
	bool HasComponent(Entity entity) const
	{
		return mRegistry.all_of<T>(entity);
	}

	template<typename T>
	T& GetComponent(Entity entity)
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "Entity does not have the component!");
		return mRegistry.get<T>(entity);
	}

private:

	entt::registry mRegistry;

};

} // namespace Gleam
