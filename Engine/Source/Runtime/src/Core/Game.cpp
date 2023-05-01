#include "gpch.h"
#include "Game.h"

#include "Window.h"
#include "Input/Input.h"
#include "World/World.h"

using namespace Gleam;

int SDLCALL Game::SDL2_EventCallback(void* data, SDL_Event* e)
{
	switch (e->type)
	{
		case SDL_QUIT:
		{
			EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
			break;
		}
		case SDL_WINDOWEVENT:
		{
			Window::EventHandler(e->window);
			break;
		}
		case SDL_KEYDOWN:
		case SDL_KEYUP:
		{
			Input::KeyboardEventHandler(e->key);
			break;
		}
		case SDL_MOUSEMOTION:
		{
			Input::MouseMoveEventHandler(e->motion);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			Input::MouseScrollEventHandler(e->wheel);
			break;
		}
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			Input::MouseButtonEventHandler(e->button);
			break;
		}
		// This case is here just to ignore this type of event to emit warning log messages
		case SDL_POLLSENTINEL:
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

Game::Game(const ApplicationProperties& props)
	: mVersion(props.appVersion), mEvent()
{
	if (mInstance == nullptr)
	{
		mInstance = this;

        // init windowing subsystem
		int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
		SDL_SetEventFilter(SDL2_EventCallback, nullptr);
		GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

		// create window
		mWindow = CreateScope<Window>(props.windowProps);
		
		// init renderer backend
		mRendererContext.ConfigureBackend(props.rendererConfig);
        
        // create world
        World::active = World::Create();

		EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
		{
			mRunning = false;
		});
	}
}

void Game::Run()
{
    Time::Reset();
	while (mRunning)
	{
		while (SDL_PollEvent(&mEvent));
        
        Time::Step();
        Input::Update();
        
        // Update engine systems
        bool fixedUpdate = Time::fixedTime <= (Time::time - Time::fixedDeltaTime);
        if (fixedUpdate)
        {
            Time::FixedStep();
            for (auto system : mSystems)
            {
                system->OnFixedUpdate();
            }
        }

		for (auto system : mSystems)
		{
            system->OnUpdate();
		}
        
        // Update world
        if (fixedUpdate)
        {
            World::active->FixedUpdate();
        }
        World::active->Update();
		
        mRendererContext.Execute(World::active->GetRenderPipeline());
	}
}

void Game::Quit()
{
    EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
}

Game::~Game()
{
	// Destroy systems
	for (auto system : mSystems)
	{
        system->OnDestroy();
	}
    mSystems.clear();
    
    // Destroy world
    World::active.reset();

	// Destroy renderer
	mRendererContext.DestroyBackend();

	// Destroy window and windowing subsystem
	mWindow.reset();
	SDL_Quit();

	mInstance = nullptr;
}
