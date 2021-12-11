#include "gpch.h"
#include "Window.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

Window::Window(const WindowProperties& props)
	: mProps(props)
{
	// query display info to create window if not provided by the user
	if (props.display.width == 0 || props.display.height == 0)
	{
		mProps.display = WindowConfig::GetCurrentDisplayMode(props.display.monitor);
	}

	// create window
	mWindow = SDL_CreateWindow(props.title.c_str(),
		SDL_WINDOWPOS_CENTERED_DISPLAY(props.display.monitor),
		SDL_WINDOWPOS_CENTERED_DISPLAY(props.display.monitor),
		props.display.width, props.display.height,
		static_cast<uint32_t>(props.windowFlag));
	GLEAM_ASSERT(mWindow, "Window creation failed!");

	// update window props with the created window info
	int monitor = SDL_GetWindowDisplayIndex(mWindow);
	GLEAM_ASSERT(monitor >= 0, "Window display index is invalid!");

	mProps.display = WindowConfig::GetCurrentDisplayMode(monitor);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		mProps.display.width = e.GetWidth();
		mProps.display.height = e.GetHeight();
		return false;
	});
}

Window::~Window()
{
	SDL_DestroyWindow(mWindow);
}
