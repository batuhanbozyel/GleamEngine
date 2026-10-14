#include "gpch.h"
#include "Engine.h"
#include "Globals.h"
#include "EventSystem.h"
#include "WindowSystem.h"
#include "Input/InputSystem.h"

using namespace Gleam;

int SDLCALL EventSystem::SDL_EventHandler(void* data, SDL_Event* e)
{
    static auto windowSystem = Globals::Engine->GetSubsystem<WindowSystem>();
    if (e->type >= SDL_EVENT_WINDOW_FIRST && e->type <= SDL_EVENT_WINDOW_LAST)
    {
        windowSystem->EventHandler(e->window);
        return 0;
    }
    
    static auto inputSystem = Globals::Engine->GetSubsystem<InputSystem>();
	if (inputSystem->ProcessEvent(*e))
	{
		return 0;
	}

	switch (e->type)
	{
		case SDL_EVENT_QUIT:
		{
			EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
			break;
		}
		// emit warning log messages
		case SDL_EVENT_POLL_SENTINEL:
		{
			break;
		}
		default:
		{
			GLEAM_CORE_WARN("Unhandled event type: {0}", e->type);
			break;
		}
	}
	return 0;
}

void EventSystem::Update()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (mEventHandler)
		{
			std::invoke(mEventHandler, &event);
		}
		SDL_EventHandler(nullptr, &event);
	}
}

void EventSystem::SetEventHandler(EventHandlerFn&& fn)
{
	mEventHandler = std::move(fn);
}
