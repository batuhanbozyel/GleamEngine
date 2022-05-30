#pragma once
#include <entt/entt.hpp>

namespace Gleam {

class GameObject;
class SceneManager;

class Scene
{
    friend class GameObject;
    friend class SceneManager;
    
public:
    
    GLEAM_NONCOPYABLE(Scene);
    
    Scene() = default;
    Scene(Scene&&) = default;
    ~Scene() = default;
    
private:
    
    entt::registry mRegistry;
    
};

} // namespace Gleam
