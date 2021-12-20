#include "gpch.h"
#include "Window.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

Window::Window(const WindowProperties& props)
	: mProperties(props)
{
	// query display info to create window if not provided by the user
	if (props.display.width == 0 || props.display.height == 0)
	{
		mProperties.windowFlag = WindowFlag::MaximizedWindow;
	}

	// create window
	mWindow = SDL_CreateWindow(mProperties.title.c_str(),
		SDL_WINDOWPOS_CENTERED_DISPLAY(mProperties.display.monitor),
		SDL_WINDOWPOS_CENTERED_DISPLAY(mProperties.display.monitor),
		mProperties.display.width, mProperties.display.height,
		static_cast<uint32_t>(mProperties.windowFlag));
	GLEAM_ASSERT(mWindow, "Window creation failed!");

	// update window props with the created window info
	int monitor = SDL_GetWindowDisplayIndex(mWindow);
	GLEAM_ASSERT(monitor >= 0, "Window display index is invalid!");

	mProperties.display = WindowConfig::GetCurrentDisplayMode(monitor);

	EventDispatcher<WindowResizeEvent>::Subscribe([this](const WindowResizeEvent& e)
	{
		mProperties.display.width = e.GetWidth();
		mProperties.display.height = e.GetHeight();
		return false;
	});
}

Window::~Window()
{
	SDL_DestroyWindow(mWindow);
}
