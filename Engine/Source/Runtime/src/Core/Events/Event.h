#pragma once
#include <list>
#include <typeindex>
#include <unordered_map>

namespace Gleam {

template<class T, class EventType>
using EventCallback = void(T::*)(EventType*);

class Event
{
public:
	virtual ~Event() = default;

	virtual TString ToString() const = 0;
};

class EventHandlerBase
{
public:

	virtual ~EventHandlerBase() = default;

	void Execute(Event* e)
	{
		Call(e);
	}

private:

	virtual void Call(Event*) = 0;
};

template<class T, class EventType>
class EventHandler : EventHandlerBase
{
public:

	EventHandler(T* instance, EventCallback<T, EventType> fn)
		: m_Instance(instance), m_Function(fn)
	{

	}

	void Call(Event* e) override
	{
		m_Instance->(*m_Function)(static_cast<EventType*>(e));
	}

private:

	T* m_Instance;
	EventCallback<T, EventType> m_Function;

};

class EventBus
{
public:

	using HandlerList = std::list<EventHandlerBase*>;

	template<typename EventType>
	void Publish(EventType* e)
	{
		HandlerList* handlers = m_Subscribers[typeid(EventType)];

		if (handlers == nullptr)
		{
			return;
		}

		for (auto& handler : *handlers)
		{
			if (handler)
			{
				handler->Execute(e);
			}
		}
	}

	template<class T, class EventType>
	void Subscribe(T* instance, EventCallback<T, EventType> fn)
	{
		HandlerList* handlers = m_Subscribers[typeid(EventType)];

		if (handlers == nullptr)
		{
			handlers = new HandlerList();
			m_Subscribers[typeid(EventType)] = handlers;
		}

		handlers->push_back(new EventHandler<T, EventType>(instance, fn));
	}

private:

	std::unordered_map<std::type_index, HandlerList*> m_Subscribers;
};

}