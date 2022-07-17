#pragma once
#include "WindowConfig.h"

namespace Gleam {

class Window final
{
public:
    
    GLEAM_NONCOPYABLE(Window);
    
	Window(const WindowProperties& props);
	~Window();

	const WindowProperties& GetProperties() const
	{
		return mProperties;
	}

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

	static void EventHandler(SDL_WindowEvent windowEvent);

private:

	SDL_Window* mWindow;
	WindowProperties mProperties;

};

} // namespace Gleam
