#pragma once
#include "Subsystem.h"
#include "EngineConfig.h"

namespace Gleam {

class Engine final
{
public:

	GLEAM_NONCOPYABLE(Engine);

	Engine() = default;

	void Initialize();

	void Shutdown();
    
    void UpdateConfig(const WindowConfig& config);
    
    void UpdateConfig(const RendererConfig& config);

	Size GetResolution() const;

	const EngineConfig& GetConfiguration() const;
    
    template<EngineSystemType T>
    T* GetSubsystem()
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
		T* system = mSubsystems.emplace<T>(std::forward<Args>(args)...);
		system->Initialize(this);
		return system;
	}

	template<EngineSystemType T>
	void RemoveSubsystem()
	{
		GLEAM_ASSERT(HasSubsystem<T>(), "Engine does not have the subsystem!");
		T* system = mSubsystems.get<T>();
		system->Shutdown();
		mSubsystems.erase<T>();
	}

    void SaveConfigToDisk() const;

	EngineConfig mConfig;
	PolyArray<EngineSubsystem> mSubsystems;

};
	
} // namespace Gleam
