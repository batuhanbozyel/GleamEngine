// EntryPoint
#include "Core/EntryPoint.h"
#include "View/ViewStack.h"

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
		auto world = worldManager->GetActiveWorld();

        auto viewStack = AddSubsystem<ViewStack>();
		viewStack->AddView<MenuBar>();
        viewStack->AddView<WorldViewport>(world);
        viewStack->AddView<WorldOutliner>(world);
		viewStack->AddView<EntityInspector>(world);
		viewStack->AddView<ContentBrowser>();
	}
    
	~GleamEditor()
	{
        RemoveSubsystem<ViewStack>();
	}

private:
    
};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance(const Gleam::CommandLine& cli)
{
    auto projectFile = Globals::StartupDirectory/"Editor.gproj";
    
    Gleam::Project project;
    if (Gleam::Filesystem::Exists(projectFile))
    {
        auto file = Gleam::File(projectFile, Gleam::FileType::Text);
        auto serializer = Gleam::JSONSerializer(file.GetStream());
        project = serializer.Deserialize<Gleam::Project>();
    }
    else
    {
        auto file = Gleam::File::Create(projectFile, Gleam::FileType::Text);
        auto serializer = Gleam::JSONSerializer(file.GetStream());
        project.name = "Gleam Editor";
        project.version = Gleam::Version(1, 0, 0);
        serializer.Serialize(project);
    }
    return new GEditor::GleamEditor(project);
}
