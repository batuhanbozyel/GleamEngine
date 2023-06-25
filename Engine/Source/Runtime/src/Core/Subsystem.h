//
//  View.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once

namespace Gleam {

class Application;

class Subsystem
{
    friend class Application;

public:
    
    virtual ~Subsystem() = default;

protected:

	virtual void Initialize() {}
    
	virtual void Shutdown() {}

};

} // namespace Gleam
