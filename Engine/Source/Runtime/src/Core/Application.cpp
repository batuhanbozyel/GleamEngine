#include "gpch.h"
#include "Application.h"
#include "WindowSystem.h"

#include "World/World.h"
#include "Input/InputSystem.h"
#include "Renderer/RenderSystem.h"

using namespace Gleam;

int SDLCALL Application::SDL2_EventHandler(void* data, SDL_Event* e)
{
    static auto windowSystem = GameInstance->GetSubsystem<WindowSystem>();
    if (e->type >= SDL_EVENT_WINDOW_FIRST && e->type <= SDL_EVENT_WINDOW_LAST)
    {
        windowSystem->EventHandler(e->window);
        return 0;
    }
    
    static auto inputSystem = GameInstance->GetSubsystem<InputSystem>();
	switch (e->type)
	{
		case SDL_EVENT_QUIT:
		{
			EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
			break;
		}
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
		{
			inputSystem->KeyboardEventHandler(e->key);
			break;
		}
		case SDL_EVENT_MOUSE_MOTION:
		{
            inputSystem->MouseMoveEventHandler(e->motion);
			break;
		}
		case SDL_EVENT_MOUSE_WHEEL:
		{
            inputSystem->MouseScrollEventHandler(e->wheel);
			break;
		}
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
		{
            inputSystem->MouseButtonEventHandler(e->button);
			break;
		}
		// This case is here just to ignore this type of event to emit warning log messages
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

Application::Application(const ApplicationProperties& props)
	: mVersion(props.version)
{
	if (mInstance == nullptr)
	{
		mInstance = this;

        // init windowing subsystem
        auto windowSubsystem = AddSubsystem<WindowSystem>();
        windowSubsystem->CreateWindow(props.windowProps);
		
		// init renderer backend
        auto renderSubsystem = AddSubsystem<RenderSystem>();
        renderSubsystem->Configure(props.rendererConfig);
        
        AddSubsystem<InputSystem>();
        
        World::active = World::Create();
        
		EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
		{
			mRunning = false;
		});
	}
}

void Application::Run()
{
    auto inputSystem = GetSubsystem<InputSystem>();
    auto renderSystem = GetSubsystem<RenderSystem>();
    
	while (mRunning)
	{
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            SDL2_EventHandler(nullptr, &event);
            if (mEventHandler)
            {
                std::invoke(mEventHandler, &event);
            }
        }
        
        inputSystem->Update();
        
        World::active->Update();
        
        renderSystem->Render();
	}
}

Application::~Application()
{
    if (mInstance)
    {
        // Destroy subsystems
        for (auto system : mSubsystems)
        {
            system->Shutdown();
        }
        mSubsystems.clear();
        
        World::active.reset();
        
        mInstance = nullptr;
    }
}
