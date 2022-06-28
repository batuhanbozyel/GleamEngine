#pragma once
#include "SceneManager.h"
#include "../Math/Transform.h"

namespace Gleam {

class GameObject
{
    entt::entity mHandle;
    
public:
    
    TString name;
	Ref<Scene> scene = SceneManager::GetActiveScene();
    Ref<Transform> transform = AddComponent<Transform>(*this);
    
    GameObject(const TString& name = "GameObject")
        : mHandle(SceneManager::GetActiveScene().mRegistry.create()), name(name)
    {
        
    }
    
    template<typename ... Types>
    GameObject(const TString& name, Types&&... components)
        : GameObject(name)
    {
        (AddComponent<Types>(components), ...);
    }
    
    ~GameObject()
    {
        scene.get().mRegistry.destroy(mHandle);
    }
    
    template<typename T, typename ... Args>
    T& AddComponent(Args&&... args) const
    {
        static_assert(std::is_base_of<Component, T>());
        return scene.get().mRegistry.emplace<T>(mHandle, std::forward<Args>(args)...);
    }
    
    template<typename T>
    T& GetComponent() const
    {
        static_assert(std::is_base_of<Component, T>());
        GLEAM_ASSERT(HasComponent<T>(), "{0} does not have component!", name);
        return scene.get().mRegistry.get<T>(mHandle);
    }

    template<typename T>
    bool HasComponent() const
    {
        static_assert(std::is_base_of<Component, T>());
        return scene.get().mRegistry.all_of<T>(mHandle);
    }

    template<typename T>
    void RemoveComponent() const
    {
        static_assert(std::is_base_of<Component, T>());
		GLEAM_ASSERT(HasComponent<T>(), "{0} does not have component!", name);
        scene.get().mRegistry.remove<T>(mHandle);
    }
    
    operator bool() const
    {
        return scene.get().mRegistry.valid(mHandle);
    }
    
};

} // namespace Gleam
