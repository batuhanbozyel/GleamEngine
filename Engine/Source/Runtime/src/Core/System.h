//
//  View.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once

namespace Gleam {

class Application;

class System
{
    friend class Application;

public:
    
    virtual ~System() = default;

protected:

	virtual void OnCreate() {}
    
	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}
    
	virtual void OnDestroy() {}

};

} // namespace Gleam
