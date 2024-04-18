#pragma once
#include "Subsystem.h"
#include "ApplicationConfig.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

class InputSystem;

template <typename T>
concept SystemType = std::is_base_of<Subsystem, T>::value;

using EventHandlerFn = std::function<void(const SDL_Event*)>;

class Application
{
public:

	GLEAM_NONCOPYABLE(Application);

	Application(const ApplicationProperties& props);
	virtual ~Application();
    
	void Run();

	template<SystemType T>
	T* AddSubsystem()
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "Application already has the subsystem!");

		T* system = mSubsystems.emplace<T>();
		if constexpr (std::is_base_of<TickableSubsystem, T>::value)
		{
			mTickableSubsystems.push_back(system);
		}
		system->mAppInstance = this;
        system->Initialize();
        return system;
	}

	template<SystemType T>
	void RemoveSubsystem()
	{
        GLEAM_ASSERT(HasSubsystem<T>(), "Application does not have the subsystem!");

        T* system = mSubsystems.get<T>();
		if constexpr (std::is_base_of<TickableSubsystem, T>::value)
		{
			mTickableSubsystems.erase(std::remove(mTickableSubsystems.begin(), mTickableSubsystems.end(), system));
		}

        system->Shutdown();
        mSubsystems.erase<T>();
	}
    
    template<SystemType T>
    T* GetSubsystem()
    {
        return mSubsystems.get<T>();
    }
    
    void SetEventHandler(EventHandlerFn&& fn)
    {
        mEventHandler = std::move(fn);
    }

	const Version& GetVersion() const
	{
		return mVersion;
	}
    
    Filesystem::path GetDefaultAssetPath() const
    {
        return Filesystem::current_path().append("Assets");
    }

	static Application* GetInstance()
	{
		return mInstance;
	}

private:

	template<SystemType T>
    bool HasSubsystem() const
    {
		return mSubsystems.contains<T>();
    }
    
    static int SDLCALL SDL2_EventHandler(void* data, SDL_Event* e);
    
    bool mRunning = true;
    
    Version mVersion;
    
    EventHandlerFn mEventHandler;
    
    PolyArray<Subsystem> mSubsystems;

	TArray<TickableSubsystem*> mTickableSubsystems;

	static inline Application* mInstance = nullptr;

};

Application* CreateApplicationInstance();

} // namespace Gleam
