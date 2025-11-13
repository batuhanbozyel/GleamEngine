// EntryPoint
#include "Core/EntryPoint.h"
#include "View/ViewStack.h"
#include "World/World.h"

#include "View/Panels/MenuBar/MenuBar.h"
#include "View/Panels/World/WorldViewport.h"
#include "View/Panels/World/WorldOutliner.h"
#include "View/Panels/Entity/EntityInspector.h"
#include "View/Panels/Project/ContentBrowser.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::Project& project)
        : Gleam::Application(project)
	{
		auto worldManager = GetSubsystem<Gleam::WorldManager>();
		mEditWorld = worldManager->GetActiveWorld();

        auto viewStack = mEditWorld->AddSubsystem<ViewStack>();
		viewStack->AddView<MenuBar>();
        viewStack->AddView<WorldViewport>();
        viewStack->AddView<WorldOutliner>();
		viewStack->AddView<EntityInspector>();
		viewStack->AddView<ContentBrowser>();
	}
    
	~GleamEditor()
	{
		mEditWorld->RemoveSubsystem<ViewStack>();
	}

private:

	Gleam::World* mEditWorld;
    
};

class GleamLauncher : public Gleam::Application
{
public:

	GleamLauncher()
		: Gleam::Application(Gleam::Project())
	{
	}

	Gleam::Project CreateProject(const Gleam::TString& name, const Gleam::Path& path = Gleam::Globals::StartupDirectory)
	{
		Gleam::Project project;
		project.name = name;
		project.path = path;
		project.version = Gleam::Version(1, 0, 0);

		auto worldRef = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
		auto worldName = Gleam::TWString(worldRef.guid.ToString()) + Gleam::World::Extension();
		auto worldFile = project.path / "Assets" / worldName;
		{
			auto file = Gleam::Filesystem::Create(worldFile, Gleam::FileType::Text);
			auto world = Gleam::World("Starter World");

			auto& camera = world.GetEntityManager().CreateEntity("Editor Camera", Gleam::Guid::NewGuid());
			world.GetEntityManager().AddComponent<Gleam::Camera>(camera, Gleam::Size(1280.0f, 720.0f));
			world.Serialize(file.GetStream());
		}
		project.worldConfig.worlds.emplace_back(worldRef);

		{
			auto filename = name;
			filename.erase(eastl::remove_if(filename.begin(), filename.end(), [](char c) { return std::isspace(c); }), filename.end());
			filename.append(".gproj");

			auto projectFile = path / filename;
			auto file = Gleam::Filesystem::Create(projectFile, Gleam::FileType::Text);
			auto serializer = Gleam::JSONSerializer();
			serializer.Serialize(project, file.GetStream());
		}
		return project;
	}

	Gleam::Project OpenProject(const Gleam::Path& path)
	{
		Gleam::Project project;
		project.path = path;
		if (Gleam::Filesystem::Exists(path))
		{
			auto file = Gleam::Filesystem::Open(path, Gleam::FileType::Text);
			auto serializer = Gleam::JSONSerializer();
			project = serializer.Deserialize<Gleam::Project>(file.GetStream());
		}
		return project;
	}
};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance(const Gleam::CommandLine& cli)
{
    Gleam::Project project;
	{
		GEditor::GleamLauncher launcer;
		auto projectFile = Globals::StartupDirectory / "GleamEditor.gproj";

		if (Gleam::Filesystem::Exists(projectFile))
		{
			project = launcer.OpenProject(projectFile);
		}
		else
		{
			project = launcer.CreateProject("Gleam Editor", Globals::StartupDirectory);
		}
	}
    return new GEditor::GleamEditor(project);
}
