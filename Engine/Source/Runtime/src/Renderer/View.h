#pragma once
#include "PipelineStateDescriptor.h"

namespace Gleam {

class Application;

class View
{
    friend class Application;

protected:

	virtual void OnCreate() {};

	virtual void OnUpdate() {};
    
    virtual void OnFixedUpdate() {};

	virtual void OnDestroy() {};

	BlendState mBlendState;

};

} // namespace Gleam
