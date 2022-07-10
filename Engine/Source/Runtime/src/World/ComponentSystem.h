#pragma once

namespace Gleam {

class World;
    
class ComponentSystem
{
    friend class World;
    
protected:
    
    virtual void OnUpdate() {}

};

} // namespace Gleam
