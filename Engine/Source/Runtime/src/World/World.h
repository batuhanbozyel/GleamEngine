#pragma once
#include "ComponentSystem.h"

namespace Gleam {

template <typename T>
concept ComponentSystemType = std::is_base_of<ComponentSystem, T>::value;

class World final
{
public:
    
	World(const TString& name = "World")
        : mName(name)
    {
        Time::Reset();
    }
    
    void Update()
    {
        Time::Step();
        
        bool fixedUpdate = Time::fixedTime <= (Time::elapsedTime - Time::fixedDeltaTime);
        if (fixedUpdate)
        {
            Time::FixedStep();
            for (auto system : mSystems)
            {
                if (system->Enabled)
                {
                    system->OnFixedUpdate(mEntityManager);
                }
            }
        }
        
        for (auto system : mSystems)
        {
            if (system->Enabled)
            {
                system->OnUpdate(mEntityManager);
            }
        }
    }
    
    template<ComponentSystemType T>
    T* AddSystem()
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        T* system = mSystems.emplace<T>();
		system->OnCreate(mEntityManager);
		return system;
    }
    
    template<ComponentSystemType T>
    void RemoveSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
		T* system = GetSystem<T>();
		system->OnDestroy(mEntityManager);
        mSystems.erase<T>();
    }
    
    template<ComponentSystemType T>
    T* GetSystem() const
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        return mSystems.get<T>();
    }
    
    template<ComponentSystemType T>
    bool HasSystem() const
    {
		return mSystems.contains<T>();
    }
    
    EntityManager& GetEntityManager()
    {
        return mEntityManager;
    }
    
private:

	TString mName;
    EntityManager mEntityManager;
    PolyArray<ComponentSystem> mSystems;

};

} // namespace Gleam
