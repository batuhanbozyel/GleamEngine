#pragma once
#include "ApplicationConfig.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

class Window;

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
	HashMap<SDL_Window*, Scope<Window>> mWindows;

};

Application* CreateApplication();

} // namespace Gleam
