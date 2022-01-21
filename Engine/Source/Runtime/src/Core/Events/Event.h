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

	bool mHandled = false;

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
		for (auto& handler : mSubscribers)
		{
			if (!e.mHandled)
			{
				e.mHandled = handler(e);
			}
		}

		e.mHandled = true;
	}

	static void Subscribe(EventHandler&& fn)
	{
		mSubscribers.emplace_back(fn);
	}

private:

	static inline TArray<EventHandler> mSubscribers;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.ToString();
}

} // namespace Gleam