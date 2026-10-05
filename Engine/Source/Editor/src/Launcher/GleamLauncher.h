#pragma once
#include "Core/Application.h"

namespace GEditor {

class GleamLauncher final : public Gleam::Application
{
public:

	GleamLauncher();

	~GleamLauncher();

	static Gleam::Project OpenProject(const Gleam::Path& path);

	static Gleam::TString ValidateNewProject(const Gleam::TString& name, const Gleam::Path& location);

	static Gleam::Path CreateProject(const Gleam::TString& name, const Gleam::Path& location);

	static void AddRecentProject(const Gleam::Path& projectFile, const Gleam::TString& name);

	static void RemoveRecentProject(const Gleam::Path& projectFile);

private:

	static void RemoveStaleProjects();

};

} // namespace GEditor
