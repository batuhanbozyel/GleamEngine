#pragma once

namespace Gleam {

class Application;
    
class Layer
{
    friend class Application;
    
protected:

	virtual void OnAttach() {};

	virtual void OnUpdate() {};
    
    virtual void OnFixedUpdate() {};

	virtual void OnRender() {};

	virtual void OnDetach() {};

};

} // namespace Gleam
