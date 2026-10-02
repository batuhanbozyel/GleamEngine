#include "ProjectBrowser.h"
#include "GleamLauncher.h"
#include "LauncherState.h"
#include "Config/EditorConfigSystem.h"
#include "View/Widgets/ThumbnailGrid.h"

#include "Core/Engine.h"
#include "Core/Globals.h"
#include "Core/Process.h"
#include "Core/Events/Event.h"
#include "Core/Events/ApplicationEvent.h"
#include "IO/FileDialog.h"
#include "Renderer/Renderers/ImGuiRenderer.h"

#include <Editor.Reflection.generated.h>

#include <imgui.h>

using namespace GEditor;

void ProjectBrowser::Render(Gleam::ImGuiRenderer* imgui)
{
	imgui->PushView([this](const Gleam::ImGuiPassData& passData)
	{
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);

		const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
		if (ImGui::Begin("Projects", nullptr, flags))
		{
			DrawHeader();
			ImGui::Separator();
			DrawRecentProjects();
			DrawNewProjectDialog();
		}
		ImGui::End();
	});
}

void ProjectBrowser::DrawHeader()
{
	ImGui::TextUnformatted("Projects");

	const float openWidth = ImGui::CalcTextSize("Open...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
	const float newWidth = ImGui::CalcTextSize("New Project...").x + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::SameLine(ImGui::GetContentRegionMax().x - openWidth - newWidth - ImGui::GetStyle().ItemSpacing.x);

	if (ImGui::Button("Open..."))
	{
		auto files = Gleam::FileDialog::Open(L"Gleam Project", L"*.gproj");
		if (files.empty() == false)
		{
			OpenProject(files.front());
		}
	}

	ImGui::SameLine();
	if (ImGui::Button("New Project..."))
	{
		mOpenNewProjectDialog = true;
	}
}

void ProjectBrowser::DrawRecentProjects()
{
	auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
	const auto& recentProjects = editorConfig->Get<Gleam::LauncherState>().recentProjects;

	Gleam::TArray<Gleam::TString> tooltips;
	Gleam::TArray<ThumbnailItem> items;
	tooltips.reserve(recentProjects.size());
	items.reserve(recentProjects.size());
	for (const auto& project : recentProjects)
	{
		const bool missing = Gleam::Filesystem::Exists(project.path) == false;
		tooltips.push_back(missing ? "Missing: " + project.path.String() : project.path.String());
		items.push_back({
			.label = project.name,
			.iconText = "Project",
			.color = Gleam::Color(0.3f, 0.6f, 0.9f, 1.0f),
			.tooltip = tooltips.back(),
			.dimmed = missing
		});
	}

	const auto events = ThumbnailGrid::Draw(items);
	if (events.doubleClicked >= 0 and items[events.doubleClicked].dimmed == false)
	{
		OpenProject(recentProjects[events.doubleClicked].path);
	}

	if (events.contextMenu >= 0)
	{
		mContextMenuProject = recentProjects[events.contextMenu].path;
		ImGui::OpenPopup("ProjectContextMenu");
	}

	if (ImGui::BeginPopup("ProjectContextMenu"))
	{
		if (ImGui::MenuItem("Remove from list"))
		{
			GleamLauncher::RemoveRecentProject(mContextMenuProject);
		}
		ImGui::EndPopup();
	}
}

void ProjectBrowser::DrawNewProjectDialog()
{
	if (mOpenNewProjectDialog)
	{
		mOpenNewProjectDialog = false;
		if (mNewProjectLocation.Empty())
		{
			auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
			const auto& recentProjects = editorConfig->Get<Gleam::LauncherState>().recentProjects;
			if (recentProjects.empty() == false)
			{
				mNewProjectLocation = recentProjects.front().path.Parent().Parent();
			}
		}
		ImGui::OpenPopup("New Project");
	}

	if (ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::InputText("Name", mNewProjectName, sizeof(mNewProjectName));

		const auto location = mNewProjectLocation.String();
		ImGui::Text("Location: %s", location.empty() ? "(none)" : location.c_str());
		ImGui::SameLine();
		if (ImGui::Button("Browse..."))
		{
			if (auto folder = Gleam::FileDialog::OpenFolder(); folder.Empty() == false)
			{
				mNewProjectLocation = folder;
			}
		}

		const Gleam::TString name = mNewProjectName;
		const auto error = GleamLauncher::ValidateNewProject(name, mNewProjectLocation);
		if (error.empty() == false)
		{
			ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", error.c_str());
		}

		ImGui::Separator();

		ImGui::BeginDisabled(error.empty() == false);
		if (ImGui::Button("Create"))
		{
			if (auto projectFile = GleamLauncher::CreateProject(name, mNewProjectLocation); projectFile.Empty() == false)
			{
				ImGui::CloseCurrentPopup();
				OpenProject(projectFile);
			}
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

void ProjectBrowser::OpenProject(const Gleam::Path& projectFile)
{
	auto project = GleamLauncher::OpenProject(projectFile);
	auto arguments = Gleam::TArray<Gleam::TString>{ "-project=" + projectFile.String() };
	GleamLauncher::AddRecentProject(projectFile, project.name);

	if (Gleam::Process::Launch(Gleam::Process::ExecutablePath(), arguments, Gleam::Filesystem::WorkingDirectory()))
	{
		Gleam::EventDispatcher<Gleam::AppCloseEvent>::Publish(Gleam::AppCloseEvent());
	}
	else
	{
		GLEAM_ERROR("Failed to launch editor for project: {0}", arguments.front());
	}
}
