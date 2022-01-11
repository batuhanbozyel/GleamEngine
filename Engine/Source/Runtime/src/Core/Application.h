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

	static const Window& GetActiveWindow()
	{
		return *sInstance->mActiveWindow;
	}

	static const Version& GetVersion()
	{
		return sInstance->mVersion;
	}

private:

	bool mRunning = true;

	Version mVersion;
	SDL_Event mEvent;
	Window* mActiveWindow;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

	static inline Application* sInstance = nullptr;

};

Application* CreateApplication();

} // namespace Gleam
