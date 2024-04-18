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

	Application* mAppInstance = nullptr;

};

class TickableSubsystem : public Subsystem
{
	friend class Application;

public:

	virtual ~TickableSubsystem() = default;

protected:

	virtual void Initialize() override {}

	virtual void Shutdown() override {}

	virtual void Tick() {}

};

} // namespace Gleam
