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
    Gleam::Project project;
    auto projectFile = Globals::StartupDirectory/"Editor.gproj";
    if (Filesystem::exists(projectFile))
    {
        auto file = Gleam::File(projectFile, Gleam::FileType::Text);
        project = Gleam::JSONSerializer::Deserialize<Gleam::Project>(file.Read());
    }
    else
    {
        project.name = "Gleam Editor";
        project.version = Gleam::Version(1, 0, 0);
        
        auto serialized = Gleam::JSONSerializer::Serialize(project);
        auto file = Gleam::File(projectFile, Gleam::FileType::Text);
        file.Write(serialized);
    }
    return new GEditor::GleamEditor(project);
}
