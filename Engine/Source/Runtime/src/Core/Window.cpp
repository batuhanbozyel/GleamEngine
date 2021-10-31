#include "gpch.h"
#include "Window.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

static int SDLCALL SDL2_EventCallback(void* data, SDL_Event* e)
{
	switch (e->type)
	{
		case SDL_QUIT:
		{
			EventDispatcher<WindowCloseEvent>::Publish(WindowCloseEvent(e->window.windowID));
			break;
		}
		case SDL_WINDOWEVENT:
		{
			switch (e->window.event)
			{
				case SDL_WINDOWEVENT_MOVED:
				{
					EventDispatcher<WindowMovedEvent>::Publish(WindowMovedEvent(e->window.windowID, e->window.data1, e->window.data2));
					break;
				}
				case SDL_WINDOWEVENT_RESIZED:
				{
					EventDispatcher<WindowResizeEvent>::Publish(WindowResizeEvent(e->window.windowID, e->window.data1, e->window.data2));
					break;
				}
				case SDL_WINDOWEVENT_MINIMIZED:
				{
					EventDispatcher<WindowMinimizeEvent>::Publish(WindowMinimizeEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_MAXIMIZED:
				{
					EventDispatcher<WindowMaximizeEvent>::Publish(WindowMaximizeEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_RESTORED:
				{
					EventDispatcher<WindowRestoreEvent>::Publish(WindowRestoreEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_ENTER:
				{
					EventDispatcher<WindowMouseEnterEvent>::Publish(WindowMouseEnterEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_LEAVE:
				{
					EventDispatcher<WindowMouseLeaveEvent>::Publish(WindowMouseLeaveEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_GAINED:
				{
					EventDispatcher<WindowFocusEvent>::Publish(WindowFocusEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_FOCUS_LOST:
				{
					EventDispatcher<WindowLostFocusEvent>::Publish(WindowLostFocusEvent(e->window.windowID));
					break;
				}
				case SDL_WINDOWEVENT_CLOSE:
				{
					EventDispatcher<WindowCloseEvent>::Publish(WindowCloseEvent(e->window.windowID));
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

Window::Window(const WindowProperties& props)
	: m_Props(props)
{
	int initSucess = SDL_Init(SDL_INIT_VIDEO);
	DEBUG_ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	// query display info to create window if not provided by the user
	if (props.Display.Width == 0 || props.Display.Height == 0)
	{
		m_Props.Display = WindowConfig::GetCurrentDisplayMode(props.Display.Monitor);
	}

	// create window
	m_Window = SDL_CreateWindow(props.Title.c_str(),
		SDL_WINDOWPOS_CENTERED_DISPLAY(props.Display.Monitor),
		SDL_WINDOWPOS_CENTERED_DISPLAY(props.Display.Monitor),
		props.Display.Width, props.Display.Height,
		static_cast<uint32_t>(props.Flag));
	DEBUG_ASSERT(m_Window, "Window creation failed!");

	SDL_SetEventFilter(SDL2_EventCallback, nullptr);

	// update window props with the created window info
	int monitor = SDL_GetWindowDisplayIndex(m_Window);
	DEBUG_ASSERT(monitor >= 0, "Window display index is invalid!");

	m_Props.Display = WindowConfig::GetCurrentDisplayMode(monitor);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		m_Props.Display.Width = e.GetWidth();
		m_Props.Display.Height = e.GetHeight();
		return false;
	});
}

Window::~Window()
{
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void Window::OnUpdate()
{
	while (SDL_PollEvent(&m_Event));
}
