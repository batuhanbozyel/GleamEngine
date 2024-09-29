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

class EngineSubsystem
{
	friend class Engine;
public:

    virtual ~EngineSubsystem() = default;

protected:

	virtual void Initialize(Engine* engine) {}
    
	virtual void Shutdown() {}

};

class GameInstanceSubsystem
{
	friend class Application;
public:

	virtual ~GameInstanceSubsystem() = default;

protected:

	virtual void Initialize(Application* app) {}

	virtual void Shutdown() {}

};

template <typename T>
concept EngineSystemType = std::is_base_of<EngineSubsystem, T>::value;

template <typename T>
concept GameInstanceSystemType = std::is_base_of<GameInstanceSubsystem, T>::value;

} // namespace Gleam
