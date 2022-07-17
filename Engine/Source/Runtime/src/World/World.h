#pragma once
#include "EntityManager.h"
#include "ComponentSystem.h"
#include <iostream>


namespace Gleam {

class World final
{
    template<typename T>
    static constexpr auto is_same_system = [](const auto& system) -> bool
    {
        return dynamic_cast<const T*>(system.get()) != nullptr;
    };
    
public:

	World(const TString& name = "World")
		: mName(name)
	{
        
	}
    
    template<typename T>
    T& AddSystem()
    {
        static_assert(std::is_base_of<ComponentSystem, T>(), "System must be derived from ComponentSystem base class!");
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        return *static_cast<T*>(mSystems.emplace_back(CreateScope<T>()).get());
    }
    
    template<typename T>
    void RemoveSystem()
    {
        static_assert(std::is_base_of<ComponentSystem, T>(), "System must be derived from ComponentSystem base class!");
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        std::erase_if(mSystems, is_same_system<T>);
    }
    
    template<typename T>
    T& GetSystem()
    {
        static_assert(std::is_base_of<ComponentSystem, T>(), "System must be derived from ComponentSystem base class!");
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        auto system = std::find_if(mSystems.begin(), mSystems.end(), is_same_system<T>);
        return *static_cast<T*>(system->get());
    }
    
    template<typename T>
    bool HasSystem() const
    {
        static_assert(std::is_base_of<ComponentSystem, T>(), "System must be derived from ComponentSystem base class!");
        return std::find_if(mSystems.begin(), mSystems.end(), is_same_system<T>) != mSystems.end();
    }
    
	EntityManager& GetEntityManager()
	{
		return mEntityManager;
	}
    
private:

	TString mName;
	EntityManager mEntityManager;
    TArray<Scope<ComponentSystem>> mSystems;

};

} // namespace Gleam
