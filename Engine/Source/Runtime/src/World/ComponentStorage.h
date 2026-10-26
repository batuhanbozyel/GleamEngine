#pragma once
#include "Container/Array.h"

#include <entt/entity/fwd.hpp>
#include <entt/entity/sparse_set.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/sigh.hpp>

#include <Reflection/Reflection.h>

#include <new>
#include <cstddef>
#include <utility>

namespace Gleam {

class TypeLifecycle;

// Every pool is created through this tag, so all component storages in a registry share one implementation
struct ComponentStorageTag {};

class ComponentStorage final : public entt::sparse_set
{
	using Base = entt::sparse_set;

	static constexpr size_t PAGE_SIZE = 1024;

public:

	using RemovedSignal = entt::sigh<void(entt::entity)>;

	explicit ComponentStorage(const allocator_type& allocator);

	~ComponentStorage() override;

	void Initialize(const TypeLifecycle& lifecycle);

	bool IsInitialized() const
	{
		return mLifecycle != nullptr;
	}

	template<typename T>
	T& Get(entt::entity entity)
	{
		return *static_cast<T*>(ElementAt(index(entity)));
	}

	template<typename T>
	const T& Get(entt::entity entity) const
	{
		return *static_cast<const T*>(ElementAt(index(entity)));
	}

	template<typename T, typename ... Args>
	T& Emplace(entt::entity entity, Args&&... args)
	{
		const auto it = Base::try_emplace(entity, false);
		return *new (AssureAtLeast(static_cast<size_t>(it.index()))) T(std::forward<Args>(args)...);
	}

	template<typename T, typename ... Args>
	T& EmplaceOrReplace(entt::entity entity, Args&&... args)
	{
		if (contains(entity))
		{
			auto& component = Get<T>(entity);
			component = T(std::forward<Args>(args)...);
			return component;
		}
		return Emplace<T>(entity, std::forward<Args>(args)...);
	}

	entt::sink<RemovedSignal> OnRemoved()
	{
		return entt::sink{ mRemoved };
	}

private:

	void* ElementAt(size_t pos) const
	{
		return mPages[pos / PAGE_SIZE] + (pos % PAGE_SIZE) * mElementSize;
	}

	void* AssureAtLeast(size_t pos);

	void DestructAll();

	void ReleasePages(size_t first);

	virtual const void* get_at(size_t pos) const override;

	virtual void swap_or_move(size_t from, size_t to) override;

	virtual void pop(basic_iterator first, basic_iterator last) override;

	virtual void pop_all() override;

	virtual basic_iterator try_emplace(entt::entity entity, bool forceBack, const void* value) override;

	virtual void reserve(size_type capacity) override;

	virtual size_type capacity() const noexcept override;

	virtual void shrink_to_fit() override;

	const TypeLifecycle* mLifecycle = nullptr;

	size_t mElementSize = 0;

	size_t mAlignment = 0;

	TArray<std::byte*> mPages;

	RemovedSignal mRemoved;
};

} // namespace Gleam

namespace entt {

template<typename EntityType, typename Allocator>
struct storage_type<Gleam::ComponentStorageTag, EntityType, Allocator>
{
	using type = Gleam::ComponentStorage;
};

} // namespace entt

namespace Gleam {

template<typename T>
constexpr uint32_t ComponentTypeHash()
{
	static_assert(Reflection::TypeHash<T>() != 0, "Components must be reflected");
	return Reflection::TypeHash<T>();
}

ComponentStorage& AssureComponentStorage(entt::registry& registry, uint32_t typeHash);

inline ComponentStorage* FindComponentStorage(entt::registry& registry, uint32_t typeHash)
{
	return static_cast<ComponentStorage*>(registry.storage(typeHash));
}

inline const ComponentStorage* FindComponentStorage(const entt::registry& registry, uint32_t typeHash)
{
	return static_cast<const ComponentStorage*>(registry.storage(typeHash));
}

} // namespace Gleam
