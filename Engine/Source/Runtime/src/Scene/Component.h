#pragma once

namespace Gleam {

class GameObject;

class Component
{
public:

	GLEAM_NONCOPYABLE(Component);

    Component(GameObject& gameObject)
        : gameObject(gameObject)
    {
        
    }
    
    Ref<GameObject> gameObject;
    
};

} // namespace Gleam
