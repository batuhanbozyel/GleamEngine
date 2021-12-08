#pragma once
#include "Window.h"
#include "Renderer/GraphicsContext.h"

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
	Scope<GraphicsContext> m_GraphicsContext;
	HashMap<SDL_Window*, Scope<Window>> m_Windows;

};

Application* CreateApplication();
}