#pragma once
#include "Entity.h"

namespace Gleam {

class EntityManager final
{    
public:

	Entity CreateEntity()
	{
		Entity entity = mRegistry.create();
		return entity;
	}

	template<typename ... Types>
	Entity CreateEntity(Types&& ... components)
	{
		Entity entity = mRegistry.create();
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
	T& AddComponent(Entity entity, Args&&... args) const
	{
		return mRegistry.emplace<T>(entity, std::forward<Args>(args)...);
	}

	template<typename T>
	T& GetComponent(Entity entity) const
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "{0} does not have component!", name);
		return mRegistry.get<T>(entity);
	}

	template<typename T>
	bool HasComponent(Entity entity) const
	{
		return mRegistry.all_of<T>(entity);
	}

	template<typename T>
	void RemoveComponent(Entity entity) const
	{
		GLEAM_ASSERT(HasComponent<T>(entity), "{0} does not have component!", name);
		mRegistry.remove<T>(entity);
	}

private:

	entt::registry mRegistry;

};

} // namespace Gleam
