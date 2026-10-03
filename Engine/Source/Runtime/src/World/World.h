#pragma once
#include "ComponentSystem.h"
#include "WorldSubsystem.h"
#include "EntityManager.h"
#include "WorldDescriptor.h"

#include "Assets/Asset.h"

#include "Container/String.h"
#include "Container/Array.h"
#include "Container/PolyArray.h"

#include "IO/Filesystem.h"

namespace Gleam {

template <typename T>
concept ComponentSystemType = std::is_base_of<ComponentSystem, T>::value;

template <typename T>
concept WorldSystemType = std::is_base_of<WorldSubsystem, T>::value;

class World final : public Asset
{
public:

	static constexpr TWStringView Extension()
	{
		return L".gworld";
	}

	World(World&&) = default;

	World(const AssetReference& reference, const AssetHeader& header, const WorldDescriptor& descriptor);

	~World();

    void Update();

	const WorldDescriptor& GetDescriptor() const
	{
		return mDescriptor;
	}

	template<WorldSystemType T, class...Args>
	T* AddSubsystem(Args&&... args)
	{
		GLEAM_ASSERT(!HasSubsystem<T>(), "World already has the subsystem!");

		T* system = mSubsystems.emplace_back<T>(std::forward<Args>(args)...);
		if constexpr (std::is_base_of<TickableWorldSubsystem, T>::value)
		{
			mTickableSubsystems.push_back(system);
		}
		system->Initialize(this);
		return system;
	}

	template<WorldSystemType T>
	void RemoveSubsystem()
	{
		GLEAM_ASSERT(HasSubsystem<T>(), "World does not have the subsystem!");

		T* system = mSubsystems.get<T>();
		if constexpr (std::is_base_of<TickableWorldSubsystem, T>::value)
		{
			mTickableSubsystems.erase(std::remove(mTickableSubsystems.begin(), mTickableSubsystems.end(), system));
		}

		system->Shutdown(this);
		mSubsystems.erase<T>();
	}

	template<WorldSystemType T>
	T* GetSubsystem() const
	{
		return mSubsystems.get<T>();
	}

	template<WorldSystemType T>
	bool HasSubsystem() const
	{
		return mSubsystems.contains<T>();
	}
    
    template<ComponentSystemType T, class...Args>
    T* AddSystem(Args&&... args)
    {
        GLEAM_ASSERT(!HasSystem<T>(), "World already has the system!");
        T* system = mSystems.emplace_back<T>(std::forward<Args>(args)...);
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
    
    const EntityManager& GetEntityManager() const
    {
        return mEntityManager;
    }

private:

	WorldDescriptor mDescriptor;
    EntityManager mEntityManager;
    PolyArray<ComponentSystem> mSystems;
	PolyArray<WorldSubsystem> mSubsystems;
	TArray<TickableWorldSubsystem*> mTickableSubsystems;

};

template<>
struct IsLoadableAsset<World> : std::false_type {};

} // namespace Gleam
