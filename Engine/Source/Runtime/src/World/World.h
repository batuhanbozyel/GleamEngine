#pragma once
#include "System.h"
#include "EntityManager.h"

namespace Gleam {

template <typename T>
concept ComponentSystemType = std::is_base_of<ISystem, T>::value;

class World final
{
public:

	World(const TString& name = "World")
		: mName(name)
	{
		
	}

	void Update()
	{
		for (auto system : mSystemManager)
        {
            system->OnUpdate(mEntityManager);
        }
	}

	void FixedUpdate()
	{
        for (auto system : mSystemManager)
        {
            system->OnFixedUpdate(mEntityManager);
        }
	}
    
    template<ComponentSystemType T>
    T* AddSystem()
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        T* system = mSystemManager.emplace<T>();
		system->OnCreate();
		return system;
    }
    
    template<ComponentSystemType T>
    void RemoveSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
		T* system = GetSystem<T>();
		system->OnDestroy();
        mSystemManager.erase<T>();
    }
    
    template<ComponentSystemType T>
    T* GetSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        mSystemManager.get<T>();
    }
    
    template<ComponentSystemType T>
    bool HasSystem() const
    {
		return mSystemManager.contains<T>();
    }
    
	EntityManager& GetEntityManager()
	{
		return mEntityManager;
	}
    
private:

	TString mName;
    PolyArray<ISystem> mSystemManager;
	EntityManager mEntityManager;

};

} // namespace Gleam
