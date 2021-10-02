#pragma once
#include <list>
#include <typeindex>
#include <functional>
#include <unordered_map>

namespace Gleam {

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

protected:

	bool m_Handled = false;
};


template<class EventType>
using EventHandler = std::function<void(Event)>;

class EventDispatcher
{
public:

	template<typename EventType>
	void Publish(Event e)
	{
		const auto& handlerIt = m_Subscribers.find(typeid(EventType));

		if (handlerIt == m_Subscribers.end())
		{
			return;
		}

		const auto& ev = static_cast<EventType>(e);
		for (auto& handler : handlerIt->second)
		{
			if (!ev.IsHandled())
			{
				handler(ev);
			}
		}
	}

	template<class EventType>
	void Subscribe(EventHandler<EventType>&& fn)
	{
		m_Subscribers[typeid(EventType)].emplace_back(fn);
	}

private:

	template<class EventType>
	using EventHandlerList = std::vector<EventHandler<EventType>>;

	std::unordered_map<std::type_index, EventHandlerList<Event>> m_Subscribers;
};

}