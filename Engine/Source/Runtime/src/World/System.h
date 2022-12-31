#pragma once

namespace Gleam {

class World;

class ISystem
{
friend class World;

protected:

	virtual void OnCreate() {};

	virtual void OnUpdate() {};

	virtual void OnFixedUpdate() {};

	virtual void OnDestroy() {};

};

} // namespace Gleam
