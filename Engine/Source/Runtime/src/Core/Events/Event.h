#pragma once

namespace Gleam {

class EventDispatcher;

/**
 * Event class
 * interface 
 */
class Event
{
public:

	virtual ~Event() = default;

	virtual TString ToString() const
	{
		TStringStream ss;
		ss << "Event";
		return ss.str();
	}

	bool IsHandled() const
	{
		return m_Handled;
	}

private:

	bool m_Handled = false;

	friend class EventDispatcher;
};

/**
 * EventPool class
 * caches heap allocations to minimize further heap allocation requests
 */
class EventPool
{
public:

	template<class EventType>
	static EventType* Allocate(const EventType& e)
	{
		const auto& poolIt = m_EventPool.find(typeid(EventType));

		EventType* newEvent;
		if (poolIt == m_EventPool.end())
		{
			newEvent = new EventType;
			memcpy(newEvent, &e, sizeof(EventType));
			m_EventPool[typeid(EventType)].push_back(newEvent);
			return newEvent;
		}
		else
		{
			for (const auto& ev : poolIt->second)
			{
				if (poolIt->second.back()->IsHandled())
				{
					newEvent = static_cast<EventType*>(poolIt->second.back());
					memcpy(newEvent, &e, sizeof(EventType));
					return newEvent;
				}
			}

			newEvent = new EventType;
			memcpy(newEvent, &e, sizeof(EventType));
			poolIt->second.push_back(newEvent);
			return newEvent;
		}
	}

private:

	static inline std::unordered_map<std::type_index, std::vector<Event*>> m_EventPool;
};

/**
 * EventDispatcher class
 * responsible from handling events
 */
class EventDispatcher
{
public:

	template<class EventType>
	using EventHandler = std::function<bool(EventType*)>;

	template<typename EventType>
	static void Publish(const EventType& e)
	{
		const auto& handlerIt = m_Subscribers.find(typeid(EventType));

		if (handlerIt == m_Subscribers.end())
		{
			return;
		}

		auto ev = EventPool::Allocate<EventType>(e);

		for (auto& handler : handlerIt->second)
		{
			if (!ev->IsHandled())
			{
				ev->m_Handled = handler(ev);
			}
		}

		ev->m_Handled = true;
	}

	template<class EventType>
	static void Subscribe(EventHandler<EventType>&& fn)
	{
		m_Subscribers[typeid(EventType)].emplace_back([fn = std::move(fn)](Event* e)
		{
			return fn(static_cast<EventType*>(e));
		});
	}

private:

	static inline std::unordered_map<std::type_index, std::vector<EventHandler<Event>>> m_Subscribers;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.ToString();
}

}