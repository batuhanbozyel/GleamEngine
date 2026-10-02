#pragma once
#include "View/View.h"
#include "IO/Path.h"
#include "Container/Array.h"
#include "Container/String.h"

namespace Gleam {
struct RecentProject;
} // namespace Gleam

namespace GEditor {

struct EditorFonts;

class ProjectBrowser final : public View
{
public:

	virtual void OnCreate(Gleam::Application* app) override;

	virtual void Render(Gleam::ImGuiRenderer* imgui) override;

private:

	void DrawSidebar();

	void DrawProjectList();

	void DrawProjectTable(const Gleam::TArray<Gleam::RecentProject>& projects, const Gleam::TArray<uint32_t>& visibleProjects);

	void DrawDetails();

	void DrawNewProjectDialog();

	void OpenProject(const Gleam::Path& projectFile);

	const EditorFonts* mFonts = nullptr;

	char mSearch[256] = "";

	int mSortMode = 0;

	Gleam::Path mSelectedProject;

	char mNewProjectName[128] = "New Project";

	Gleam::Path mNewProjectLocation;

	bool mOpenNewProjectDialog = false;

};

} // namespace GEditor
