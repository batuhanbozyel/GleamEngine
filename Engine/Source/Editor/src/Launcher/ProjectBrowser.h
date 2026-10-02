#pragma once
#include "View/View.h"
#include "IO/Path.h"
#include "Container/String.h"

namespace GEditor {

class ProjectBrowser final : public View
{
public:

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawHeader();

	void DrawRecentProjects();

	void DrawNewProjectDialog();

	void OpenProject(const Gleam::Path& projectFile);

	char mNewProjectName[128] = "New Project";

	Gleam::Path mNewProjectLocation;

	Gleam::Path mContextMenuProject;

	bool mOpenNewProjectDialog = false;

};

} // namespace GEditor
