#pragma once
#include "Window.h"
#include "ApplicationConfig.h"

#include "Renderer/View.h"
#include "Renderer/RendererContext.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

template <typename T>
concept ViewType = std::is_base_of<View, T>::value;

class Application
{
public:

	GLEAM_NONCOPYABLE(Application);

	Application(const ApplicationProperties& props);
	virtual ~Application();

	void Run();

	template<ViewType T>
	T& PushView()
	{
		GLEAM_ASSERT(!HasView<T>(), "Application already has the view!");
		T& view = mViews.emplace<T>(&mRendererContext);
		view.OnCreate();
		return view;
	}

	template<ViewType T>
	T& PushOverlay()
	{
		GLEAM_ASSERT(!HasView<T>(), "Application already has the overlay!");
		T& overlay = mOverlays.emplace<T>(&mRendererContext);
		overlay.OnCreate();
		return overlay;
	}

	template<ViewType T>
	void RemoveView()
	{
		GLEAM_ASSERT(HasView<T>(), "Application does not have the view!");
		T& view = mViews.get_unsafe<T>();
		view.OnDestroy();
        mViews.erase<T>();
	}

	template<ViewType T>
	void RemoveOverlay()
	{
		GLEAM_ASSERT(HasOverlay<T>(), "Application does not have the overlay!");
		T& overlay = mOverlays.get<T>();
		overlay.OnDestroy();
        mOverlays.erase<T>();
	}

	const Window& GetActiveWindow() const
	{
		return *mActiveWindow;
	}

	const Version& GetVersion() const
	{
		return mVersion;
	}
    
    Filesystem::path GetDefaultAssetPath() const
    {
        return Filesystem::current_path().append("Assets/");
    }

	static Application& GetInstance()
	{
		return *mInstance;
	}

	Color backgroundColor = Color{0.1f, 0.1f, 0.1f, 1.0f};

private:

	template<ViewType T>
    bool HasView() const
    {
		return mViews.contains<T>();
    }

	template<ViewType T>
    bool HasOverlay() const
    {
		return mOverlays.contains<T>();
    }

	AnyArray mViews;
	AnyArray mOverlays;

	SDL_Event mEvent;
	Window* mActiveWindow;
	HashMap<SDL_Window*, Scope<Window>> mWindows;

	RendererContext mRendererContext;

	bool mRunning = true;
	Version mVersion;

	static inline Application* mInstance = nullptr;
    static int SDLCALL SDL2_EventCallback(void* data, SDL_Event* e);

};

Application* CreateApplication();

} // namespace Gleam
