//
//  EWorldManager.h
//  Editor
//

#pragma once
#include "Core/Subsystem.h"
#include "IO/Path.h"

namespace Gleam {
class World;
} // namespace Gleam

namespace GEditor {

class EWorldManager final : public Gleam::GameInstanceSubsystem
{
public:

	EWorldManager(Gleam::World* world);

	void Save();

	void SaveAs(const Gleam::Path& file);

	Gleam::World* GetEditWorld() const
	{
		return mEditWorld;
	}

private:

	Gleam::World* mEditWorld = nullptr;

};

} // namespace GEditor
