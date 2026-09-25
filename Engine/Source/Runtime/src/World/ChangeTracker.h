#pragma once
#include "Core/EngineDefines.h"
#include "Container/Array.h"
#include "Container/Hash.h"

#include <entt/core/type_info.hpp>
#include <entt/entity/entity.hpp>

#include <Reflection/Reflection.h>
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

namespace Gleam {

class Entity;

using Tick = uint64_t;

template<typename T>
struct IsTrackedComponent : std::true_type {};

template<>
struct IsTrackedComponent<Entity> : std::false_type {};

struct ComponentTicks
{
	entt::entity entity = entt::null;
	Tick added = 0;
	Tick changed = 0;
};

struct RemovedRecord
{
	entt::entity entity = entt::null;
	Tick tick = 0;
};

class ChangeTracker final
{
public:

	Tick GetTick() const
	{
		return mTick;
	}

	void EndFrame()
	{
		const Tick horizon = mFrameTicks[mFrameIndex];
		mFrameTicks[mFrameIndex] = mTick;
		mFrameIndex ^= 1u;
		CollectGarbage(horizon);
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
	void MarkRemoved(entt::entity entity)
	{
		if constexpr (IsTrackedComponent<T>::value)
		{
			MarkRemoved(TypeHashOf<T>(), entity);
		}
	}

	void MarkRemoved(uint32_t typeHash, entt::entity entity)
	{
		++mTick;
		mRemoved[typeHash].push_back(RemovedRecord{ .entity = entity, .tick = mTick });

		auto it = mTicks.find(typeHash);
		if (it != mTicks.end())
		{
			const uint32_t index = entt::to_entity(entity);
			if (index < it->second.size() and it->second[index].entity == entity)
			{
				it->second[index] = ComponentTicks{};
			}
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

	template<typename T, typename Func>
	void ForEachRemoved(Tick since, Func&& fn) const
	{
		const auto it = mRemoved.find(TypeHashOf<T>());
		if (it != mRemoved.end())
		{
			for (const auto& record : it->second)
			{
				if (record.tick > since)
				{
					fn(record.entity);
				}
			}
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

	void CollectGarbage(Tick horizon)
	{
		for (auto& [typeHash, queue] : mRemoved)
		{
			size_t keepFrom = 0;
			while (keepFrom < queue.size() and queue[keepFrom].tick <= horizon)
			{
				++keepFrom;
			}
			queue.erase(queue.begin(), queue.begin() + keepFrom);
		}
	}

	Tick mTick = 0;
	Tick mFrameTicks[2] = { 0, 0 };
	uint32_t mFrameIndex = 0;
	HashMap<uint32_t, TArray<ComponentTicks>> mTicks;
	HashMap<uint32_t, TArray<RemovedRecord>> mRemoved;
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
