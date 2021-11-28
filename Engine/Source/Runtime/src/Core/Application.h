#pragma once
#include "Window.h"

#ifdef __cplusplus
extern "C"
#endif

namespace Gleam {

class Application
{
public:

	Application(const WindowProperties& props);
	~Application();

	void Run();

private:

	bool m_Running = true;

	SDL_Event m_Event;
	HashMap<SDL_Window*, Scope<Window>> m_Windows;

};

Application* CreateApplication();
}