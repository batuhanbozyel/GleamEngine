#include "GleamLauncher.h"
#include "LauncherState.h"
#include "ProjectBrowser.h"
#include "Config/EditorConfigSystem.h"
#include "EWorld/EWorldManager.h"
#include "View/ViewStack.h"

#include "Serialization/JSONSerializer.h"
#include "World/World.h"
#include "World/Components/Camera.h"
#include "World/Components/SkyAtmosphere.h"
#include "World/Components/ReflectionProbe.h"

#include "Core/Engine.h"
#include "Core/WindowSystem.h"
#include "Core/Globals.h"

#include <Editor.Reflection.generated.h>

#include <chrono>

using namespace GEditor;

GleamLauncher::GleamLauncher()
	: Gleam::Application(Gleam::Project{ .name = "Gleam Launcher" })
{
	auto windowSystem = Gleam::Globals::Engine->GetSubsystem<Gleam::WindowSystem>();
	windowSystem->SetTransientWindow(Gleam::Size(1280.0f, 800.0f), false);

	auto editorConfig = Gleam::Globals::Engine->AddSubsystem<EditorConfigSystem>();
	editorConfig->Register<Gleam::LauncherState>();

	auto viewStack = AddSubsystem<ViewStack>("Launcher.ini");
	viewStack->AddView<ProjectBrowser>();
}

GleamLauncher::~GleamLauncher()
{
	RemoveSubsystem<ViewStack>();
	Gleam::Globals::Engine->RemoveSubsystem<EditorConfigSystem>();
}

void GleamLauncher::AddRecentProject(const Gleam::Path& projectFile, const Gleam::TString& name)
{
	const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();

	auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
	editorConfig->Modify<Gleam::LauncherState>([projectFile, name, now](Gleam::LauncherState& state)
	{
		auto& projects = state.recentProjects;
		projects.erase(eastl::remove_if(projects.begin(), projects.end(), [&](const Gleam::RecentProject& project)
		{
			return project.path == projectFile;
		}), projects.end());
		projects.insert(projects.begin(), Gleam::RecentProject{ .path = projectFile, .name = name, .lastOpened = static_cast<uint64_t>(now) });
	});
}

void GleamLauncher::RemoveRecentProject(const Gleam::Path& projectFile)
{
	auto editorConfig = Gleam::Globals::Engine->GetSubsystem<EditorConfigSystem>();
	editorConfig->Modify<Gleam::LauncherState>([projectFile](Gleam::LauncherState& state)
	{
		auto& projects = state.recentProjects;
		projects.erase(eastl::remove_if(projects.begin(), projects.end(), [&](const Gleam::RecentProject& project)
		{
			return project.path == projectFile;
		}), projects.end());
	});
}

Gleam::Project GleamLauncher::OpenProject(const Gleam::Path& path)
{
	Gleam::Project project;
	if (Gleam::Filesystem::Exists(path))
	{
		auto file = Gleam::Filesystem::OpenRead(path, Gleam::FileType::Text);
		auto serializer = Gleam::JSONSerializer();
		project = serializer.Deserialize<Gleam::Project>(file->GetStream());
	}
	project.path = path.Parent();
	return project;
}

Gleam::TString GleamLauncher::ValidateNewProject(const Gleam::TString& name, const Gleam::Path& location)
{
	if (name.empty())
	{
		return "Project name is empty";
	}
	else if (name.find_first_of("\\/:*?\"<>|") != Gleam::TString::npos)
	{
		return "Project name cannot contain \\ / : * ? \" < > |";
	}
	else if (name.front() == ' ' or name.back() == ' ' or name.back() == '.')
	{
		return "Project name cannot start or end with a space, or end with a dot";
	}
	else if (location.Empty() or Gleam::Filesystem::IsDirectory(location) == false)
	{
		return "Location is not an existing folder";
	}

	const auto projectDirectory = location / name;
	if (Gleam::Filesystem::Exists(projectDirectory) and Gleam::Filesystem::IsDirectory(projectDirectory) == false)
	{
		return "A file with the project name already exists in the location";
	}

	bool empty = true;
	Gleam::Filesystem::ForEach(projectDirectory, [&](const Gleam::DirectoryEntry& entry)
	{
		empty = false;
	}, false);
	if (empty == false)
	{
		return "A non-empty folder with the project name already exists in the location";
	}
	return Gleam::TString();
}

Gleam::Path GleamLauncher::CreateProject(const Gleam::TString& name, const Gleam::Path& location)
{
	if (auto error = ValidateNewProject(name, location); error.empty() == false)
	{
		GLEAM_ERROR("Project could not be created: {0}", error);
		return Gleam::Path();
	}

	const auto projectDirectory = location / name;
	const auto contentDirectory = projectDirectory / "Assets";
	if (Gleam::Filesystem::CreateDirectories(contentDirectory) == false)
	{
		return Gleam::Path();
	}

	Gleam::Project project;
	project.name = name;
	project.path = projectDirectory;
	project.version = Gleam::Version(1, 0, 0);

	auto worldRef = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
	auto worldName = Gleam::TWString(worldRef.guid.ToString()) + Gleam::World::Extension();
	auto worldFile = contentDirectory / worldName;
	{
		auto world = Gleam::World(worldRef, Gleam::AssetHeader{
			.typeGuid = Gleam::Reflection::GetClass<Gleam::WorldDescriptor>().Guid(),
			.name = "Starter World"
		}, Gleam::WorldDescriptor{});

		auto& camera = world.GetEntityManager().CreateEntity("Main Camera", Gleam::Guid::NewGuid());
		world.GetEntityManager().AddComponent<Gleam::Camera>(camera, Gleam::Size(1280.0f, 720.0f), Gleam::ProjectionType::Perspective);

		auto& atmosphere = world.GetEntityManager().CreateEntity("Atmosphere", Gleam::Guid::NewGuid());
		world.GetEntityManager().AddComponent<Gleam::SkyAtmosphere>(atmosphere);

		// global probe
		world.GetEntityManager().SetSingleton<Gleam::ReflectionProbe>();

		EWorldManager worldManager(&world);
		worldManager.SaveAs(worldFile);
	}
	project.worldConfig.worlds.emplace_back(worldRef);

	auto filename = name;
	filename.erase(eastl::remove_if(filename.begin(), filename.end(), [](char c) { return std::isspace(c); }), filename.end());
	filename.append(".gproj");

	auto projectFile = projectDirectory / filename;
	{
		auto file = Gleam::Filesystem::Create(projectFile, Gleam::FileType::Text);
		auto serializer = Gleam::JSONSerializer();
		serializer.Serialize(project, file->GetStream());
	}
	return projectFile;
}
