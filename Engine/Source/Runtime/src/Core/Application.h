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

	bool mRunning = true;

	SDL_Event mEvent;
	Scope<GraphicsContext> mGraphicsContext;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

};

Application* CreateApplication();

} // namespace Gleam