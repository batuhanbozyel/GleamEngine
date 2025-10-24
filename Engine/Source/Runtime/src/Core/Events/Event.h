#pragma once
#include "Container/String.h"
#include "Container/Array.h"
#include "IO/Log.h"

#include <functional>
#include <ostream>

#define EventHandlerFn(fn) [](auto&&... args) -> bool { return fn(); }

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
    
	virtual TString ToString() const = 0;
    
};

/**
 * EventDispatcher class
 * responsible for handling events
 */
template <class EventType>
class EventDispatcher
{
public:
    
	using EventHandler = std::function<void(EventType)>;

	static void Publish(EventType e)
	{
		GLEAM_CORE_TRACE("{}", e.ToString());
		for (auto& handler : mSubscribers)
		{
            handler(e);
		}
	}

	static void Subscribe(const EventHandler& fn)
	{
		mSubscribers.push_back(fn);
	}
    
    static void Unsubscribe(const EventHandler& fn)
    {
        auto it = std::find(mSubscribers.begin(), mSubscribers.end(), fn);
        if (it != mSubscribers.end())
            mSubscribers.erase(it);
    }

private:

	static inline TArray<EventHandler> mSubscribers;
};

inline std::ostream& operator<<(std::ostream& os, const Event& e)
{
	return os << e.ToString();
}

} // namespace Gleam
