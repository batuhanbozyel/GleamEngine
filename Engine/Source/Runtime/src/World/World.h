#pragma once
#include "EntityManager.h"
#include "ComponentSystem.h"

namespace Gleam {

template<typename T>
concept ComponentSystemType = std::same_as<T, ComponentSystem>;
    
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
    
    template<ComponentSystemType T>
    T& AddSystem()
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        return *static_cast<T*>(mSystems.emplace_back(CreateScope<T>()).get());
    }
    
    template<ComponentSystemType T>
    void RemoveSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        std::erase_if(mSystems, is_same_system<T>);
    }
    
    template<ComponentSystemType T>
    T& GetSystem()
    {
        GLEAM_ASSERT(HasSystem<T>(), "World does not have the system!");
        auto system = std::find_if(mSystems.begin(), mSystems.end(), is_same_system<T>);
        return *static_cast<T*>(system->get());
    }
    
    template<ComponentSystemType T>
    bool HasSystem() const
    {
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
