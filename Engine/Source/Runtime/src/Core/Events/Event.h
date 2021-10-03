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
using EventHandler = std::function<void(const SharedPtr<EventType>&)>;

class EventDispatcher
{
public:

	template<typename EventType>
	static void Publish(const SharedPtr<Event>& e)
	{
		const auto& handlerIt = m_Subscribers.find(typeid(EventType));

		if (handlerIt == m_Subscribers.end())
		{
			return;
		}

		auto ev = std::static_pointer_cast<EventType>(e);
		for (auto& handler : handlerIt->second)
		{
			if (!ev->IsHandled())
			{
				handler(ev);
			}
		}
	}

	template<class EventType>
	static void Subscribe(EventHandler<EventType>&& fn)
	{
		m_Subscribers[typeid(EventType)].emplace_back([fn = std::move(fn)](const SharedPtr<Event>& e)
		{
			fn(std::static_pointer_cast<EventType>(e));
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