// EntryPoint
#include "Core/EntryPoint.h"
#include "View/ViewStack.h"

#include "View/Panels/MenuBar/MenuBar.h"
#include "View/Panels/World/WorldViewport.h"
#include "View/Panels/World/WorldOutliner.h"
#include "View/Panels/Project/ContentBrowser.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::ApplicationProperties& properties)
        : Gleam::Application(properties)
	{
        auto viewStack = Gleam::World::active->AddSystem<ViewStack>();
		viewStack->AddView<MenuBar>();
        viewStack->AddView<WorldViewport>();
        viewStack->AddView<WorldOutliner>();
		viewStack->AddView<ContentBrowser>();
	}
    
	~GleamEditor()
	{
        Gleam::World::active->RemoveSystem<ViewStack>();
	}

private:
    
};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance()
{
    Gleam::ApplicationProperties props;
    props.version = Gleam::Version(1, 0, 0);
    props.windowConfig.title = "Gleam Editor";
    props.windowConfig.windowFlag = Gleam::WindowFlag::CustomWindow;
	props.windowConfig.size.width = 1280;
	props.windowConfig.size.height = 720;
    return new GEditor::GleamEditor(props);
}
