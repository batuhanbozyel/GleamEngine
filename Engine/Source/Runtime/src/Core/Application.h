#pragma once
#include "Window.h"
#include "ApplicationConfig.h"

#ifdef __cplusplus
extern "C"
#endif

namespace Gleam {

class GraphicsContext;

class Application
{
public:

	Application(const ApplicationProperties& props);
	~Application();

	void Run();

	const Version& GetVersion() const
	{
		return mVersion;
	}

private:

	bool mRunning = true;

	Version mVersion;
	SDL_Event mEvent;
	Scope<GraphicsContext> mGraphicsContext;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

};

Application* CreateApplication();

} // namespace Gleam