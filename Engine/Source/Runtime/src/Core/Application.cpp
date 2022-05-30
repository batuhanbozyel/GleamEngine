#include "gpch.h"
#include "Application.h"

#include "Layer.h"
#include "Window.h"
#include "Events/WindowEvent.h"
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
			switch (e->window.event)
			{
				case SDL_WINDOWEVENT_MOVED:
				{
					EventDispatcher<WindowMovedEvent>::Publish(WindowMovedEvent(SDL_GetWindowFromID(e->window.windowID), e->window.data1, e->window.data2));
					break;
				}
				case SDL_WINDOWEVENT_RESIZED:
				{
					EventDispatcher<WindowResizeEvent>::Publish(WindowResizeEvent(SDL_GetWindowFromID(e->window.windowID), e->window.data1, e->window.data2));
					break;
				}
				case SDL_WINDOWEVENT_MINIMIZED:
				{
					EventDispatcher<WindowMinimizeEvent>::Publish(WindowMinimizeEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_MAXIMIZED:
				{
					EventDispatcher<WindowMaximizeEvent>::Publish(WindowMaximizeEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_RESTORED:
				{
					EventDispatcher<WindowRestoreEvent>::Publish(WindowRestoreEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_ENTER:
				{
					EventDispatcher<WindowMouseEnterEvent>::Publish(WindowMouseEnterEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_LEAVE:
				{
					EventDispatcher<WindowMouseLeaveEvent>::Publish(WindowMouseLeaveEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					EventDispatcher<WindowFocusEvent>::Publish(WindowFocusEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					EventDispatcher<WindowLostFocusEvent>::Publish(WindowLostFocusEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
				case SDL_WINDOWEVENT_CLOSE:
				{
					EventDispatcher<WindowCloseEvent>::Publish(WindowCloseEvent(SDL_GetWindowFromID(e->window.windowID)));
					break;
				}
			}
			break;
		}
		default:
		{
			GLEAM_CORE_WARN("Unhandled event type!");
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
        if (Time::fixedTime <= (Time::time - Time::fixedDeltaTime))
        {
            Time::FixedStep();
            for (const auto& layer : mLayerStack)
            {
                layer->OnFixedUpdate();
            }

            for (const auto& overlay : mOverlays)
            {
                overlay->OnFixedUpdate();
            }
        }
        
        for (const auto& layer : mLayerStack)
        {
            layer->OnUpdate();
        }

        for (const auto& overlay : mOverlays)
        {
            overlay->OnUpdate();
        }
		
#ifdef USE_METAL_RENDERER
        @autoreleasepool
#endif
        {
            Renderer::BeginFrame();
            
            for (const auto& layer : mLayerStack)
            {
                layer->OnRender();
            }

            for (const auto& overlay : mOverlays)
            {
                overlay->OnRender();
            }
            
            Renderer::EndFrame();
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

void Application::PushLayer(const RefCounted<Layer>& layer)
{
	layer->OnAttach();
	mLayerStack.push_back(layer);
}

void Application::PushOverlay(const RefCounted<Layer>& overlay)
{
	overlay->OnAttach();
	mOverlays.push_back(overlay);
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
