#pragma once
#include "Window.h"
#include "System.h"
#include "ApplicationConfig.h"
#include "Renderer/RenderPipeline.h"
#include "Renderer/RendererContext.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

template <typename T>
concept SystemType = std::is_base_of<System, T>::value;

using EventHandlerFn = std::function<void(const SDL_Event*)>;

class Application
{
public:

	GLEAM_NONCOPYABLE(Application);

	Application(const ApplicationProperties& props);
	virtual ~Application();

	void Run();
    
    void Quit();

	template<SystemType T>
	T* AddSystem()
	{
		GLEAM_ASSERT(!HasSystem<T>(), "Application already has the system!");
		T* system = mSystems.emplace<T>();
        system->OnCreate();
		return system;
	}

	template<SystemType T>
	void RemoveSystem()
	{
		GLEAM_ASSERT(HasSystem<T>(), "Application does not have the system!");
		T* system = mSystems.get<T>();
        system->OnDestroy();
        mSystems.erase<T>();
	}
    
    template<SystemType T>
    T* GetSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "Application does not have the system!");
        return mSystems.get<T>();
    }
    
    void SetEventHandler(EventHandlerFn&& fn)
    {
        mEventHandler = std::move(fn);
    }

	RenderPipeline* GetRenderPipeline()
    {
        return mRenderPipeline.get();
    }
    
    const RenderPipeline* GetRenderPipeline() const
    {
        return mRenderPipeline.get();
    }

	const Window* GetWindow() const
	{
		return mWindow.get();
	}

	const Version& GetVersion() const
	{
		return mVersion;
	}
    
    const RendererConfig& GetRendererConfig() const
    {
        return mRendererContext.GetConfiguration();
    }
    
    Filesystem::path GetDefaultAssetPath() const
    {
        return Filesystem::current_path().append("Assets/");
    }

	static Application* GetInstance()
	{
		return mInstance;
	}

	Color backgroundColor = Color{0.1f, 0.1f, 0.1f, 1.0f};

private:

	template<SystemType T>
    bool HasSystem() const
    {
		return mSystems.contains<T>();
    }

	PolyArray<System> mSystems;
    
    Scope<Window> mWindow;
    
    Scope<RenderPipeline> mRenderPipeline;
    
    EventHandlerFn mEventHandler;

	RendererContext mRendererContext;

	bool mRunning = true;
	Version mVersion;

	static inline Application* mInstance = nullptr;
    static int SDLCALL SDL2_EventHandler(void* data, SDL_Event* e);

};

Application* CreateApplicationInstance();

} // namespace Gleam
