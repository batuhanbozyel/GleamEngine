//
//  ScriptReflection.h
//  Editor
//

#pragma once
#include "ScriptLifecycles.h"
#include "Core/Subsystem.h"
#include "Core/Project.h"
#include "IO/Filesystem.h"

namespace Gleam {
class EntityManager;
} // namespace Gleam

namespace GEditor {

class ProjectSolution;

class ScriptReflection final : public Gleam::EngineSubsystem
{
public:

	explicit ScriptReflection(const Gleam::Project& project);

	virtual void Shutdown(Gleam::Engine* engine) override;

	bool Refresh(Gleam::TArrayView<Gleam::EntityManager* const> entityManagers);

	ProjectSolution& GetSolution() const
	{
		return *mSolution;
	}

private:

	void Reload(Gleam::TArrayView<Gleam::EntityManager* const> entityManagers);

	ProjectSolution* mSolution = nullptr;

	ScriptLifecycles mLifecycles;

	Gleam::FileTime mDatabaseTime;
};

} // namespace GEditor
