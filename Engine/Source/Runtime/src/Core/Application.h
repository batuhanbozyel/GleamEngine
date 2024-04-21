#pragma once
#include "Project.h"

namespace Gleam {

class Engine;
class CommandLine;

class Application
{
	friend class Engine;
public:

	GLEAM_NONCOPYABLE(Application);

	Application(const Project& project);

	virtual ~Application();
    
	void Run();

	template<SystemType T, class...Args>
	T* AddSubsystem(Args&&... args)
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "Application already has the subsystem!");

		T* system = mSubsystems.emplace<T>(std::forward<Args>(args)...);
		if constexpr (std::is_base_of<TickableSubsystem, T>::value)
		{
			mTickableSubsystems.push_back(system);
		}
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

	template<SystemType T>
    bool HasSubsystem() const
    {
		return mSubsystems.contains<T>();
    }

private:
    
    bool mRunning = true;
    
    Project mProject;

	PolyArray<Subsystem> mSubsystems;

	TArray<TickableSubsystem*> mTickableSubsystems;

};

Application* CreateApplicationInstance(const CommandLine& cli);

} // namespace Gleam
