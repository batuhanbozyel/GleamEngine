#include "gpch.h"
#include "Window.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

Window::Window(const WindowProperties& props)
	: m_Props(props)
{
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
	GLEAM_ASSERT(m_Window, "Window creation failed!");

	// update window props with the created window info
	int monitor = SDL_GetWindowDisplayIndex(m_Window);
	GLEAM_ASSERT(monitor >= 0, "Window display index is invalid!");

	m_Props.Display = WindowConfig::GetCurrentDisplayMode(monitor);
}

Window::~Window()
{
	SDL_DestroyWindow(m_Window);
}

void Window::Resize(uint32_t width, uint32_t height)
{
	m_Props.Display.Width = width;
	m_Props.Display.Height = height;
}
