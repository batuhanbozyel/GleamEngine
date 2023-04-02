#pragma once
#include "EntityManager.h"
#include "ComponentSystem.h"

namespace Gleam {

template <typename T>
concept ComponentSystemType = std::is_base_of<ComponentSystem, T>::value;

class World final
{
public:

	World(const TString& name = "World");

	void Update();

	void FixedUpdate();
    
    template<ComponentSystemType T>
    T* AddSystem()
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        T* system = mSystemManager.emplace<T>();
		system->OnCreate(mEntityManager);
		return system;
    }
    
    template<ComponentSystemType T>
    void RemoveSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
		T* system = GetSystem<T>();
		system->OnDestroy(mEntityManager);
        mSystemManager.erase<T>();
    }
    
    template<ComponentSystemType T>
    T* GetSystem() const
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        mSystemManager.get<T>();
    }
    
    template<ComponentSystemType T>
    bool HasSystem() const
    {
		return mSystemManager.contains<T>();
    }

	static RefCounted<World> Create()
    {
        return CreateRef<World>();
    }

    static inline RefCounted<World> active = Create();
    
private:

	TString mName;
    EntityManager mEntityManager;
    PolyArray<ComponentSystem> mSystemManager;

};

} // namespace Gleam
