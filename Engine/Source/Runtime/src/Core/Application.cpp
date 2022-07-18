#include "gpch.h"
#include "Application.h"

#include "Layer.h"
#include "Window.h"
#include "Input/Input.h"
#include "Renderer/Renderer.h"

using namespace Gleam;

static int SDLCALL SDL2_EventCallback(void* data, SDL_Event* e)
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
		case SDL_TEXTEDITING:
		case SDL_TEXTINPUT:
		case SDL_KEYMAPCHANGED:
		{
			Input::KeyboardEventHandler(e->key);
			break;
		}
		case SDL_MOUSEMOTION:
		{
			Input::MouseMotionEventHandler(e->motion);
			break;
		}
		case SDL_MOUSEWHEEL:
		{
			Input::MouseWheelEventHandler(e->wheel);
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
	sInstance = this;

	int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetEventFilter(SDL2_EventCallback, nullptr);
	GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	mActiveWindow = new Window(props.windowProps);
	mWindows.emplace(mActiveWindow->GetSDLWindow(), Scope<Window>(mActiveWindow));
	Renderer::Init(props.windowProps.title, props.appVersion, props.rendererProps);

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

void Application::Run()
{
    Time::Reset();
	while (mRunning)
	{
		while (SDL_PollEvent(&mEvent));
        
        Time::Step();

		for (const auto& layer : mLayerStack)
		{
			layer->OnUpdate();
		}

		bool fixedUpdate = Time::fixedTime <= (Time::time - Time::fixedDeltaTime);
        if (fixedUpdate)
        {
            Time::FixedStep();
            for (const auto& layer : mLayerStack)
            {
                layer->OnFixedUpdate();
            }
        }

        for (const auto& overlay : mOverlays)
        {
            overlay->OnUpdate();
        }

		if (fixedUpdate)
		{
			for (const auto& overlay : mOverlays)
			{
				overlay->OnFixedUpdate();
			}
		}
		
#ifdef USE_METAL_RENDERER
        @autoreleasepool
#endif
        {
            for (const auto& layer : mLayerStack)
            {
                layer->OnRender();
            }

            for (const auto& overlay : mOverlays)
            {
                overlay->OnRender();
            }
        }
	}
}

Application::~Application()
{
	// Destroy overlays
	for (const auto& overlay : mOverlays)
	{
		overlay->OnDetach();
	}
	mOverlays.clear();

	// Destroy layers
	for (const auto& layer : mLayerStack)
	{
		layer->OnDetach();
	}
	mLayerStack.clear();

	// Destroy renderer
	Renderer::Destroy();

	// Destroy windows and window subsystem
	mWindows.clear();
	SDL_Quit();

	sInstance = nullptr;
}

uint32_t Application::PushLayer(const RefCounted<Layer>& layer)
{
    uint32_t index = mLayerStack.size();
	layer->OnAttach();
	mLayerStack.push_back(layer);
    return index;
}

uint32_t Application::PushOverlay(const RefCounted<Layer>& overlay)
{
    uint32_t index = mOverlays.size();
	overlay->OnAttach();
	mOverlays.push_back(overlay);
    return index;
}

void Application::RemoveLayer(uint32_t index)
{
	GLEAM_ASSERT(index < mLayerStack.size());
	mLayerStack[index]->OnDetach();
	mLayerStack.erase(mLayerStack.begin() + index);
}

void Application::RemoveOverlay(uint32_t index)
{
	GLEAM_ASSERT(index < mOverlays.size());
	mOverlays[index]->OnDetach();
	mOverlays.erase(mOverlays.begin() + index);
}

void Application::RemoveLayer(const RefCounted<Layer>& layer)
{
	auto layerIt = std::find(mLayerStack.begin(), mLayerStack.end(), layer);
	if (layerIt != mLayerStack.end())
	{
		layer->OnDetach();
		mLayerStack.erase(layerIt);
	}
}

void Application::RemoveOverlay(const RefCounted<Layer>& overlay)
{
	auto overlayIt = std::find(mOverlays.begin(), mOverlays.end(), overlay);
	if (overlayIt != mOverlays.end())
	{
		overlay->OnDetach();
		mOverlays.erase(overlayIt);
	}
}
