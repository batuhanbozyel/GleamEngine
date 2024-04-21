#pragma once
#include "Subsystem.h"
#include "EngineConfig.h"

namespace Gleam {

class Engine final
{
public:

	void Initialize();

	void Shutdown();
    
    void UpdateConfig(const WindowConfig& config);
    
    void UpdateConfig(const RendererConfig& config);

	template<SystemType T, class...Args>
	T* AddSubsystem(Args&&... args)
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "Engine already has the subsystem!");
		T* system = mSubsystems.emplace<T>(std::forward<Args>(args)...);
        system->Initialize();
        return system;
	}

	template<SystemType T>
	void RemoveSubsystem()
	{
        GLEAM_ASSERT(HasSubsystem<T>(), "Engine does not have the subsystem!");
        T* system = mSubsystems.get<T>();
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
    
    Size GetResolution() const;

    const EngineConfig& GetConfiguration() const;

private:
    
    void SaveConfigToDisk() const;

	EngineConfig mConfig;
	PolyArray<Subsystem> mSubsystems;

};
	
} // namespace Gleam
