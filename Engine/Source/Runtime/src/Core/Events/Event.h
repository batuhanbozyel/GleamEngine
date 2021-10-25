#pragma once

namespace Gleam {

template<class EventType>
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

private:

	bool m_Handled = false;

	template<class EventType>
	friend class EventDispatcher;
};

/**
 * EventDispatcher class
 * responsible from handling events
 */
template <class EventType>
class EventDispatcher
{
public:

	using EventHandler = std::function<bool(EventType)>;

	static void Publish(EventType e)
	{
		for (auto& handler : m_Subscribers)
		{
			if (!e.m_Handled)
			{
				e.m_Handled = handler(e);
			}
		}

		e.m_Handled = true;
	}

	static void Subscribe(EventHandler&& fn)
	{
		m_Subscribers.emplace_back(fn);
	}

private:

	static inline TArray<EventHandler> m_Subscribers;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.ToString();
}

}