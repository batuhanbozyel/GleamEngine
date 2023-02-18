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
		for (auto& system : mSystemManager)
        {
			auto& sys = std::any_cast<ISystem&>(system);
            sys.OnUpdate(mEntityManager);
        }
	}

	void FixedUpdate()
	{
		for (auto& system : mSystemManager)
        {
			auto& sys = std::any_cast<ISystem&>(system);
            sys.OnFixedUpdate(mEntityManager);
        }
	}
    
    template<ComponentSystemType T>
    T& AddSystem()
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        T& system = mSystemManager.emplace<T>();
		system.OnCreate();
		return system;
    }
    
    template<ComponentSystemType T>
    void RemoveSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
		T& system = mSystemManager.get<T>();
		system.OnDestroy();
        mSystemManager.erase<T>();
    }
    
    template<ComponentSystemType T>
    T& GetSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        return mSystemManager.get<T>();
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
	AnyArray mSystemManager;
	EntityManager mEntityManager;

};

} // namespace Gleam
