#include "gpch.h"
#include "Application.h"

#include "Window.h"
#include "Input/Input.h"
#include "World/World.h"

#include "Renderer/Renderers/WorldRenderer.h"
#include "Renderer/Renderers/PostProcessStack.h"

using namespace Gleam;

int SDLCALL Application::SDL2_EventCallback(void* data, SDL_Event* e)
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

Application::Application(const ApplicationProperties& props)
	: mVersion(props.appVersion)
{
	if (mInstance == nullptr)
	{
		mInstance = this;

        // init windowing subsystem
		int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
		GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

		// create window
		mWindow = CreateScope<Window>(props.windowProps);
		
		// init renderer backend
		mRendererContext.ConfigureBackend(props.rendererConfig);
        mRenderPipeline = CreateScope<RenderPipeline>();
        
        // create world
        World::active = World::Create();
        mRenderPipeline->AddRenderer<WorldRenderer>();
        mRenderPipeline->AddRenderer<PostProcessStack>();

		EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
		{
			mRunning = false;
		});
	}
}

void Application::Run()
{
    Time::Reset();
	while (mRunning)
	{
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            SDL2_EventCallback(nullptr, &event);
            if (mEventHandler)
            {
                std::invoke(mEventHandler, &event);
            }
        }
        
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
		
        mRendererContext.Execute(mRenderPipeline.get());
	}
}

void Application::Quit()
{
    EventDispatcher<AppCloseEvent>::Publish(AppCloseEvent());
}

Application::~Application()
{
    if (mInstance)
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
        mRenderPipeline.reset();
        mRendererContext.DestroyBackend();
        
        // Destroy window and windowing subsystem
        mWindow.reset();
        SDL_Quit();
        
        mInstance = nullptr;
    }
}
