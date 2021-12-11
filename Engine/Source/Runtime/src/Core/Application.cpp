#include "gpch.h"
#include "Application.h"

#include "Window.h"
#include "Events/WindowEvent.h"
#include "Renderer/GraphicsContext.h"

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
	Log::Init();

	int initSucess = SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetEventFilter(SDL2_EventCallback, nullptr);
	GLEAM_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	Window* mainWindow = new Window(props.windowProps);
	mWindows.emplace(mainWindow->GetSDLWindow(), Scope<Window>(mainWindow));
	mGraphicsContext = CreateScope<GraphicsContext>(mainWindow->GetSDLWindow(), props.windowProps.title, props.appVersion);

	EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
	{
		mRunning = false;
		return true;
	});

	EventDispatcher<WindowCloseEvent>::Subscribe([this](WindowCloseEvent e)
	{
		mWindows.erase(e.GetWindow());
		return true;
	});
}

void Application::Run()
{
	while (mRunning)
	{
		while (SDL_PollEvent(&mEvent));
	}
}

Application::~Application()
{
	mGraphicsContext.reset();
	mWindows.clear();
	SDL_Quit();
}
