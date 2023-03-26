//
//  View.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once

namespace Gleam {

class Game;

class System
{
    friend class Game;

public:
    
    virtual ~System() = default;

protected:

	virtual void OnCreate() {}
    
	virtual void OnUpdate() {}
    
    virtual void OnFixedUpdate() {}
    
	virtual void OnDestroy() {}

};

} // namespace Gleam
