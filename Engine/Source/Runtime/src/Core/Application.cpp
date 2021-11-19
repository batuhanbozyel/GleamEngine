#include "gpch.h"
#include "Application.h"

#include "Events/WindowEvent.h"

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

Application::Application(const WindowProperties& props)
{
	Log::Init();

	int initSucess = SDL_Init(SDL_INIT_VIDEO);
	SDL_SetEventFilter(SDL2_EventCallback, nullptr);
	DEBUG_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	Scope<Window> mainWindow = CreateScope<Window>(props);
	m_Windows.emplace(mainWindow->GetSDLWindow(), std::move(mainWindow));

	EventDispatcher<AppCloseEvent>::Subscribe([this](AppCloseEvent e)
	{
		m_Running = false;
		return true;
	});

	EventDispatcher<WindowCloseEvent>::Subscribe([this](WindowCloseEvent e)
	{
		m_Windows.erase(e.GetWindow());
		return true;
	});
}

void Application::Run()
{
	while (m_Running)
	{
		while (SDL_PollEvent(&m_Event));
	}
}

Application::~Application()
{
	m_Windows.clear();
	SDL_Quit();
}
