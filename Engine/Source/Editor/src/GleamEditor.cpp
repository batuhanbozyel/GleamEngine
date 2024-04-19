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
    project.name = "Gleam Editor";
    project.engineConfig.version = Gleam::Version(1, 0, 0);
    project.engineConfig.window.windowFlag = Gleam::WindowFlag::MaximizedWindow;
    return new GEditor::GleamEditor(project);
}
