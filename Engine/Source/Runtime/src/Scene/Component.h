#pragma once

namespace Gleam {

class GameObject;

class Component
{
public:
    
    Component(GameObject& gameObject)
        : gameObject(gameObject)
    {
        
    }
    
    Component(Component&) = delete;
    Component& operator=(Component&) = delete;
    
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;
    
    Ref<GameObject> gameObject;
    
};

} // namespace Gleam
