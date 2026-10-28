#pragma once
#include "LaunchFailureDialog.h"
#include "Core/Application.h"

namespace GEditor {

class GleamLauncher final : public Gleam::Application
{
public:

	explicit GleamLauncher(const LaunchFailure& failure = LaunchFailure());

	~GleamLauncher();

	static Gleam::TString ValidateNewProject(const Gleam::TString& name, const Gleam::Path& location);

	static Gleam::Path CreateProject(const Gleam::TString& name, const Gleam::Path& location);

	static void AddRecentProject(const Gleam::Path& projectFile, const Gleam::TString& name);

	static void RemoveRecentProject(const Gleam::Path& projectFile);

private:

	static void RemoveStaleProjects();

};

} // namespace GEditor
