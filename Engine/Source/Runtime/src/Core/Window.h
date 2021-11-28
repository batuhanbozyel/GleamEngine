#pragma once
#include <SDL.h>
#include "WindowConfig.h"

namespace Gleam {

class Window
{
public:

	Window(const WindowProperties& props);
	~Window();

	const WindowProperties& GetProps() const
	{
		return m_Props;
	}

	SDL_Window* GetSDLWindow() const
	{
		return m_Window;
	}

	void Resize(uint32_t width, uint32_t height);

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;

private:

	SDL_Window* m_Window;
	WindowProperties m_Props;

};

}