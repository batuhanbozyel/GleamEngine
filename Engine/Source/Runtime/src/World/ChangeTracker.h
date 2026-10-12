#pragma once
#include "Core/EngineDefines.h"
#include "Container/Array.h"
#include "Container/Hash.h"

#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>
#include <entt/signal/sigh.hpp>

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

class Entity;
class EntityManager;

using Tick = uint64_t;

template<typename T>
struct IsTrackedComponent : std::true_type {};

template<>
struct IsTrackedComponent<Entity> : std::false_type {};

struct ComponentCallback final
{
	friend class EntityManager;

public:

	ComponentCallback() = default;

	ComponentCallback(const ComponentCallback&) = delete;

	ComponentCallback& operator=(const ComponentCallback&) = delete;

	ComponentCallback(ComponentCallback&& other) noexcept
		: mConnection(other.mConnection)
	{
		other.mConnection = {};
	}

	ComponentCallback& operator=(ComponentCallback&& other) noexcept
	{
		if (this != &other)
		{
			Reset();
			mConnection = other.mConnection;
			other.mConnection = {};
		}
		return *this;
	}

	~ComponentCallback()
	{
		Reset();
	}

	void Reset()
	{
		if (mConnection)
		{
			mConnection.release();
		}
	}

	bool IsConnected() const
	{
		return static_cast<bool>(mConnection);
	}

private:

	explicit ComponentCallback(entt::connection connection)
		: mConnection(connection)
	{

	}

	entt::connection mConnection = {};
};

struct ComponentTicks
{
	entt::entity entity = entt::null;
	Tick added = 0;
	Tick changed = 0;
};

class ChangeTracker;

struct ComponentTicksView final
{
	friend class ChangeTracker;

public:

	ComponentTicksView() = default;

	bool IsAdded(entt::entity entity, Tick since) const
	{
		const auto* entry = Find(entity);
		if (entry == nullptr)
		{
			return false;
		}
		else
		{
			return entry->added > since;
		}
	}

	bool IsChanged(entt::entity entity, Tick since) const
	{
		const auto* entry = Find(entity);
		if (entry == nullptr)
		{
			return false;
		}
		else
		{
			return entry->changed > since;
		}
	}

private:

	explicit ComponentTicksView(TArrayView<const ComponentTicks> ticks)
		: mTicks(ticks)
	{

	}

	const ComponentTicks* Find(entt::entity entity) const
	{
		const uint32_t index = entt::to_entity(entity);
		if (index >= mTicks.size() or mTicks[index].entity != entity)
		{
			return nullptr;
		}
		else
		{
			return &mTicks[index];
		}
	}

	TArrayView<const ComponentTicks> mTicks = {};
};

class ChangeTracker final
{
public:

	Tick GetTick() const
	{
		return mTick;
	}

	template<typename T>
	void MarkAdded(entt::entity entity)
	{
		if constexpr (IsTrackedComponent<T>::value)
		{
			MarkAdded(TypeHashOf<T>(), entity);
		}
	}

	void MarkAdded(uint32_t typeHash, entt::entity entity)
	{
		++mTick;
		auto& entry = GetOrCreateTicks(typeHash, entity);
		entry.added = mTick;
		entry.changed = mTick;
	}

	template<typename T>
	void MarkChanged(entt::entity entity)
	{
		if constexpr (IsTrackedComponent<T>::value)
		{
			MarkChanged(TypeHashOf<T>(), entity);
		}
	}

	void MarkChanged(uint32_t typeHash, entt::entity entity)
	{
		++mTick;
		GetOrCreateTicks(typeHash, entity).changed = mTick;
	}

	template<typename T>
	ComponentTicksView GetTicks() const
	{
		const auto it = mTicks.find(TypeHashOf<T>());
		if (it == mTicks.end())
		{
			return ComponentTicksView{};
		}
		else
		{
			return ComponentTicksView{ TArrayView<const ComponentTicks>{ it->second.data(), it->second.size() } };
		}
	}

	template<typename T>
	bool IsAdded(entt::entity entity, Tick since) const
	{
		const auto* entry = FindTicks(TypeHashOf<T>(), entity);
		if (entry == nullptr)
		{
			return false;
		}
		else
		{
			return entry->added > since;
		}
	}

	template<typename T>
	bool IsChanged(entt::entity entity, Tick since) const
	{
		const auto* entry = FindTicks(TypeHashOf<T>(), entity);
		if (entry == nullptr)
		{
			return false;
		}
		else
		{
			return entry->changed > since;
		}
	}

private:

	template<typename T>
	static uint32_t TypeHashOf()
	{
		if constexpr (Reflection::Traits::IsReflected<T>::value)
		{
			return Reflection::GetClass<T>().TypeHash();
		}
		else
		{
			return static_cast<uint32_t>(entt::type_hash<T>::value());
		}
	}

	ComponentTicks& GetOrCreateTicks(uint32_t typeHash, entt::entity entity)
	{
		auto& ticks = mTicks[typeHash];
		const uint32_t index = entt::to_entity(entity);
		if (index >= ticks.size())
		{
			ticks.resize(index + 1);
		}

		auto& entry = ticks[index];
		if (entry.entity != entity)
		{
			entry = ComponentTicks{};
			entry.entity = entity;
		}
		return entry;
	}

	const ComponentTicks* FindTicks(uint32_t typeHash, entt::entity entity) const
	{
		const auto it = mTicks.find(typeHash);
		if (it == mTicks.end())
		{
			return nullptr;
		}
		else
		{
			const uint32_t index = entt::to_entity(entity);
			if (index >= it->second.size() or it->second[index].entity != entity)
			{
				return nullptr;
			}
			else
			{
				return &it->second[index];
			}
		}
	}

	Tick mTick = 0;
	HashMap<uint32_t, TArray<ComponentTicks>> mTicks;
};

class ChangeCursor final
{
public:

	Tick Begin(const ChangeTracker& tracker)
	{
		mPending = tracker.GetTick();
		return mLastSeen;
	}

	void Commit()
	{
		mLastSeen = mPending;
	}

	Tick GetLastSeen() const
	{
		return mLastSeen;
	}

private:

	Tick mLastSeen = 0;
	Tick mPending = 0;
};

} // namespace Gleam
