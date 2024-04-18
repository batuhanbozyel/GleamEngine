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

	GleamEditor(const Gleam::ApplicationProperties& properties)
        : Gleam::Application(properties)
	{
        auto viewStack = AddSubsystem<ViewStack>();
		viewStack->AddView<MenuBar>();
        viewStack->AddView<WorldViewport>();
        viewStack->AddView<WorldOutliner>();
		viewStack->AddView<EntityInspector>();
		viewStack->AddView<ContentBrowser>();
	}
    
	~GleamEditor()
	{
        RemoveSubsystem<ViewStack>();
	}

private:
    
};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance()
{
    Gleam::ApplicationProperties props;
    props.version = Gleam::Version(1, 0, 0);
    props.windowConfig.title = "Gleam Editor";
    props.windowConfig.windowFlag = Gleam::WindowFlag::MaximizedWindow;
    return new GEditor::GleamEditor(props);
}
