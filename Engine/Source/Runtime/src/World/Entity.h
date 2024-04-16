#pragma once
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

namespace Gleam {

using EntityHandle = entt::entity;
static constexpr EntityHandle InvalidEntity = entt::null;

template<typename ... Excludes>
using Exclude = entt::exclude_t<Excludes...>;

class Entity
{
public:
    
    Entity() = default;
    
    Entity(EntityHandle handle, entt::registry* registry = nullptr)
		: mHandle(handle), mRegistry(registry)
	{

	}

	template<typename T, typename ... Args>
	T& AddComponent(Args&&... args)
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(!HasComponent<T>(), "Entity already has the component!");
		return mRegistry->emplace<T>(mHandle, std::forward<Args>(args)...);
	}
    
    template<typename T, typename ... Args>
    void SetComponent(Args&&... args)
    {
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
        GLEAM_ASSERT(!HasComponent<T>(), "Entity already has the component!");
        mRegistry->emplace_or_replace<T>(mHandle, std::forward<Args>(args)...);
    }

	template<typename T>
	void RemoveComponent()
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
		mRegistry->remove<T>(mHandle);
	}

	template<typename T>
	bool HasComponent() const
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		return mRegistry->all_of<T>(mHandle);
	}

	template<typename T>
	T& GetComponent()
	{
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
		GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
		return mRegistry->get<T>(mHandle);
	}
    
    template<typename T>
    const T& GetComponent() const
    {
        GLEAM_ASSERT(IsValid(), "Entity is invalid!");
        GLEAM_ASSERT(HasComponent<T>(), "Entity does not have the component!");
        return mRegistry->get<T>(mHandle);
    }
    
    bool IsValid() const
    {
        return mHandle != InvalidEntity && mRegistry != nullptr;
    }
    
	operator EntityHandle() const
	{
		return mHandle;
	}

private:
    
    EntityHandle mHandle = InvalidEntity;
    
    entt::registry* mRegistry = nullptr;
};

} // namespace Gleam
