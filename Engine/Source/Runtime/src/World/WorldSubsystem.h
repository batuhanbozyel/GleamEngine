#pragma once

namespace Gleam {

class WorldSubsystem
{
	friend class World;
public:

    virtual ~WorldSubsystem() = default;

protected:

	virtual void Initialize(World* world) {}
    
	virtual void Shutdown() {}

};

class TickableWorldSubsystem : public WorldSubsystem
{
	friend class World;
public:

	virtual ~TickableWorldSubsystem() = default;

protected:

	virtual void Initialize(World* world) override {}

	virtual void Shutdown() override {}

	virtual void Tick() {}

};

} // namespace Gleam
