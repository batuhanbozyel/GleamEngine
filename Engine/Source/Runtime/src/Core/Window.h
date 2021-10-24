#pragma once
#include "WindowConfig.h"

namespace Gleam {

class Window
{
public:

	Window(const WindowProperties& props);

	~Window();

	void OnUpdate();

	const WindowProperties& GetProps() const
	{
		return m_Props;
	}

private:

	WindowProperties m_Props;
	SDL_Window* m_Window;
	SDL_Event m_Event;

};

}