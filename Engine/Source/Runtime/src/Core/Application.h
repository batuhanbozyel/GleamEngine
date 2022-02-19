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

	void PushLayer(const Ref<Layer>& layer);

	void PushOverlay(const Ref<Layer>& overlay);

	void RemoveLayer(uint32_t index);

	void RemoveOverlay(uint32_t index);

	void RemoveLayer(const Ref<Layer>& layer);

	void RemoveOverlay(const Ref<Layer>& overlay);

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

	TArray<Ref<Layer>> mLayerStack;
	TArray<Ref<Layer>> mOverlays;

	SDL_Event mEvent;
	Window* mActiveWindow;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

	bool mRunning = true;
	Version mVersion;

	static inline Application* sInstance = nullptr;

};

Application* CreateApplication();

} // namespace Gleam
