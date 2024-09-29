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

	template<GameInstanceSystemType T, class...Args>
	T* AddSubsystem(Args&&... args)
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "Application already has the subsystem!");

		T* system = mSubsystems.emplace<T>(std::forward<Args>(args)...);
        system->Initialize(this);
        return system;
	}

	template<GameInstanceSystemType T>
	void RemoveSubsystem()
	{
        GLEAM_ASSERT(HasSubsystem<T>(), "Application does not have the subsystem!");

        T* system = mSubsystems.get<T>();
        system->Shutdown();
        mSubsystems.erase<T>();
	}
    
    template<GameInstanceSystemType T>
    T* GetSubsystem()
    {
        return mSubsystems.get<T>();
    }

	template<GameInstanceSystemType T>
    bool HasSubsystem() const
    {
		return mSubsystems.contains<T>();
    }

private:
    
    bool mRunning = true;
    
    Project mProject;

	PolyArray<GameInstanceSubsystem> mSubsystems;

};

Application* CreateApplicationInstance(const CommandLine& cli);

} // namespace Gleam
