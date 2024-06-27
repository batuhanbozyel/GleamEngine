//
//  Subsystem.h
//  Runtime
//
//  Created by Batuhan Bozyel on 31.10.2022.
//

#pragma once

namespace Gleam {

class Engine;
class Application;

class Subsystem
{
	friend class Engine;
    friend class Application;
public:

    virtual ~Subsystem() = default;

protected:

	virtual void Initialize() {}
    
	virtual void Shutdown() {}

};

class TickableSubsystem : public Subsystem
{
	friend class Engine;
	friend class Application;
public:

	virtual ~TickableSubsystem() = default;

protected:

	virtual void Initialize() override {}

	virtual void Shutdown() override {}

	virtual void Tick() {}

};

template <typename T>
concept SystemType = std::is_base_of<Subsystem, T>::value;

} // namespace Gleam
