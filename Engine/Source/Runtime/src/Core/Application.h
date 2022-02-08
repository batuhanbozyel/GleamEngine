#pragma once
#include "ApplicationConfig.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

class Layer;
class Window;

class Application
{
public:

	Application(const ApplicationProperties& props);
	~Application();

	void Run();

	void PushLayer(Layer* layer);

	void PushOverlay(Layer* overlay);

	const Window& GetActiveWindow() const
	{
		return *mActiveWindow;
	}

	const Version& GetVersion() const
	{
		return mVersion;
	}

	static Application& GetInstance()
	{
		return *sInstance;
	}

private:

	TArray<Scope<Layer>> mLayerStack;
	TArray<Scope<Layer>> mOverlays;

	SDL_Event mEvent;
	Window* mActiveWindow;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

	bool mRunning = true;
	Version mVersion;

	static inline Application* sInstance = nullptr;

};

Application* CreateApplication();

} // namespace Gleam
