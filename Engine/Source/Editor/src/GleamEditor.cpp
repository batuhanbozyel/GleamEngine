#define GDEBUG
// EntryPoint
#include "Core/EntryPoint.h"
#include "View/ViewStack.h"
#include "View/Panels/Scene/SceneView.h"
#include "View/Panels/MenuBar/MenuBarView.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::ApplicationProperties& properties)
        : Gleam::Application(properties)
	{
        auto viewStack = GameInstance->AddSystem<ViewStack>();
		viewStack->AddView<MenuBarView>();
        viewStack->AddView<SceneView>();
	}

	~GleamEditor()
	{
        GameInstance->RemoveSystem<ViewStack>();
	}

private:
    
};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplicationInstance()
{
    Gleam::ApplicationProperties props;
    props.appVersion = Gleam::Version(1, 0, 0);
    props.windowProps.title = "Gleam Editor";
    props.windowProps.windowFlag = Gleam::WindowFlag::MaximizedWindow;
    return new GEditor::GleamEditor(props);
}
