#include "gpch.h"
#include "Globals.h"
#include "Application.h"
#include "WindowSystem.h"

#include "Reflection/Database.h"
#include "Serialization/JSONSerializer.h"

#include "Input/InputSystem.h"
#include "World/WorldManager.h"
#include "Renderer/RenderSystem.h"
#include "Renderer/Material/MaterialSystem.h"

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

Application::Application(const Project& project)
	: mVersion(project.engineConfig.version)
{
	if (mInstance == nullptr)
	{
		mInstance = this;

		// setup globals
		Globals::ProjectName = project.name;
		Globals::ProjectDirectory = project.path;
		Globals::StartupDirectory = Filesystem::current_path();
		Globals::BuiltinAssetsDirectory = Globals::StartupDirectory/"Assets";
		Globals::ProjectContentDirectory = Globals::ProjectDirectory/"Assets";
        
        // init reflection & serialization
        AddSubsystem<Reflection::Database>();
        AddSubsystem<JSONSerializer>();
        
        // init windowing subsystem
        auto windowSubsystem = AddSubsystem<WindowSystem>();
        windowSubsystem->Configure(project.engineConfig.window);
		
		// init renderer backend
        auto renderSubsystem = AddSubsystem<RenderSystem>();
        renderSubsystem->Configure(project.engineConfig.renderer);
		renderSubsystem->ResetRenderTarget();
        
		// init world manager
		auto worldManager = AddSubsystem<WorldManager>();
		worldManager->Configure(project.worldConfig);

        AddSubsystem<InputSystem>();
        AddSubsystem<MaterialSystem>();
        
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
	auto worldManager = GetSubsystem<WorldManager>();
    
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

		for (auto system : mTickableSubsystems)
		{
			system->Tick();
		}
        
		auto world = worldManager->GetActiveWorld();
        world->Update();

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
		mTickableSubsystems.clear();
        
        mInstance = nullptr;
    }
}
