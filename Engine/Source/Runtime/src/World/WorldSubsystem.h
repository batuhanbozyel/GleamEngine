#pragma once

namespace Gleam {

class World;

class WorldSubsystem
{
	friend class World;
public:

    virtual ~WorldSubsystem() = default;

protected:

	virtual void Initialize(World* world) {}
    
	virtual void Shutdown(World* world) {}

};

class TickableWorldSubsystem : public WorldSubsystem
{
	friend class World;
public:

	virtual ~TickableWorldSubsystem() = default;

protected:

	virtual void Initialize(World* world) override {}

	virtual void Shutdown(World* world) override {}

	virtual void Tick(World* world) {}

};

} // namespace Gleam
