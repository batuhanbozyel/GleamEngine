#include "gpch.h"
#include "Application.h"

#include "Window.h"
#include "Input/Input.h"
#include "Renderer/View.h"

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
	: mVersion(props.appVersion), mEvent()
{
	if (mInstance == nullptr)
	{
		mInstance = this;

		int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
		SDL_SetEventFilter(SDL2_EventCallback, nullptr);
		GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

		// init main window
		mActiveWindow = new Window(props.windowProps);
		mWindows.emplace(mActiveWindow->GetSDLWindow(), Scope<Window>(mActiveWindow));
		
		// init renderer backend
		mRendererContext.ConfigureBackend(props.windowProps.title, props.appVersion, props.rendererConfig);

		EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
		{
			mRunning = false;
			return true;
		});

		EventDispatcher<WindowCloseEvent>::Subscribe([this](WindowCloseEvent e)
		{
			// if there is only 1 window, application should terminate with the proper deallocation order
			if (mWindows.size() == 1)
			{
				return false;
			}

			mWindows.erase(e.GetWindow());
			return true;
		});
	}
}

void Application::Run()
{
    Time::Reset();
	while (mRunning)
	{
		while (SDL_PollEvent(&mEvent));
        
        Time::Step();
        Input::Update();

		for (auto view : mViews)
		{
			view->OnUpdate();
		}

		bool fixedUpdate = Time::fixedTime <= (Time::time - Time::fixedDeltaTime);
        if (fixedUpdate)
        {
            Time::FixedStep();
            for (auto view : mViews)
            {
                view->OnFixedUpdate();
            }
        }

        for (auto overlay : mOverlays)
        {
            overlay->OnUpdate();
        }

		if (fixedUpdate)
		{
			for (auto overlay : mOverlays)
			{
                overlay->OnFixedUpdate();
			}
		}
		
#ifdef USE_METAL_RENDERER
		@autoreleasepool
#endif
		{
            for (auto view : mViews)
            {
                view->OnRender();
                mRendererContext.Exectute(view->mRenderPipeline);
            }

            for (auto overlay : mOverlays)
            {
                overlay->OnRender();
                mRendererContext.Exectute(overlay->mRenderPipeline);
            }
        }
	}
}

Application::~Application()
{
	// Destroy overlays
	for (auto overlay : mOverlays)
	{
		overlay->OnDestroy();
	}
	mOverlays.clear();

	// Destroy views
	for (auto view : mViews)
	{
		view->OnDestroy();
	}
	mViews.clear();

	// Destroy renderer
	mRendererContext.DestroyBackend();

	// Destroy windows and window subsystem
	mWindows.clear();
	SDL_Quit();

	mInstance = nullptr;
}
