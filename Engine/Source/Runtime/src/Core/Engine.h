#pragma once
#include "Subsystem.h"
#include "CommandLine.h"
#include "EngineConfig.h"
#include "EngineDefines.h"
#include "Container/PolyArray.h"

namespace Gleam {

class Engine final
{
public:

	GLEAM_NONCOPYABLE(Engine);

	Engine() = default;

	void Initialize(const CommandLine& cli);

	void Shutdown();

    template<EngineSystemType T>
    T* GetSubsystem()
    {
        return mSubsystems.get<T>();
    }

    template<EngineSystemType T>
    const T* GetSubsystem() const
    {
        return mSubsystems.get<T>();
    }

	template<EngineSystemType T>
    bool HasSubsystem() const
    {
		return mSubsystems.contains<T>();
    }

private:

	template<EngineSystemType T, class...Args>
	T* AddSubsystem(Args&&... args)
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "Engine already has the subsystem!");
		T* system = mSubsystems.emplace_back<T>(std::forward<Args>(args)...);
		system->Initialize(this);
		return system;
	}

	template<EngineSystemType T>
	void RemoveSubsystem()
	{
		GLEAM_ASSERT(HasSubsystem<T>(), "Engine does not have the subsystem!");
		T* system = mSubsystems.get<T>();
		system->Shutdown(this);
		mSubsystems.erase<T>();
	}

	PolyArray<EngineSubsystem> mSubsystems;
};
	
} // namespace Gleam
