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

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance(const Gleam::CommandLine& cli)
{
    auto projectFile = Globals::StartupDirectory/"Editor.gproj";
    
    Gleam::Project project;
    if (Gleam::Filesystem::Exists(projectFile))
    {
        auto file = Gleam::Filesystem::Open(projectFile, Gleam::FileType::Text);
        auto serializer = Gleam::JSONSerializer();
        project = serializer.Deserialize<Gleam::Project>(file.GetStream());
    }
    else
    {
		project.name = "Gleam Editor";
		project.version = Gleam::Version(1, 0, 0);

		auto worldRef = Gleam::AssetReference{ .guid = Gleam::Guid::NewGuid() };
		auto worldName = worldRef.guid.ToString() + Gleam::World::Extension().data();
		auto worldFile = Globals::StartupDirectory/project.path/"Assets"/worldName;
		{
			auto file = Gleam::Filesystem::Create(worldFile, Gleam::FileType::Text);
			auto world = Gleam::World("Starter World");

			auto& camera = world.GetEntityManager().CreateEntity(Gleam::Guid::NewGuid());
			world.GetEntityManager().AddComponent<Gleam::Camera>(camera, Gleam::Size(1280.0f, 720.0f));

			world.Serialize(file.GetStream());
		}
		project.worldConfig.worlds.emplace_back(worldRef);

		{
			auto file = Gleam::Filesystem::Create(projectFile, Gleam::FileType::Text);
			auto serializer = Gleam::JSONSerializer();
			serializer.Serialize(project, file.GetStream());
		}
    }
    return new GEditor::GleamEditor(project);
}
