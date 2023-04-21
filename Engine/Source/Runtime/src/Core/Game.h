#pragma once
#include "Window.h"
#include "System.h"
#include "ApplicationConfig.h"
#include "Renderer/RendererContext.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

template <typename T>
concept SystemType = std::is_base_of<System, T>::value;

class Game
{
public:

	GLEAM_NONCOPYABLE(Game);

	Game(const ApplicationProperties& props);
	virtual ~Game();

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

	static Game* GetInstance()
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

	SDL_Event mEvent;
    Scope<Window> mWindow;

	RendererContext mRendererContext;

	bool mRunning = true;
	Version mVersion;

	static inline Game* mInstance = nullptr;
    static int SDLCALL SDL2_EventCallback(void* data, SDL_Event* e);

};

Game* CreateGameInstance();

} // namespace Gleam
