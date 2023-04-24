#define GDEBUG
// EntryPoint
#include "Core/EntryPoint.h"
#include "View/ViewStack.h"
#include "View/SceneView/SceneView.h"

namespace GEditor {

class GleamEditor : public Gleam::Game
{
public:

	GleamEditor(const Gleam::ApplicationProperties& properties)
        : Gleam::Game(properties)
	{
        auto viewStack = GameInstance->AddSystem<ViewStack>();
        viewStack->AddView<SceneView>();
	}

	~GleamEditor()
	{
        GameInstance->RemoveSystem<ViewStack>();
	}

private:
    
};

} // namespace GEditor

Gleam::Game* Gleam::CreateGameInstance()
{
    Gleam::ApplicationProperties props;
    props.appVersion = Gleam::Version(1, 0, 0);
    props.windowProps.title = "Gleam Editor";
    props.windowProps.windowFlag = Gleam::WindowFlag::MaximizedWindow;
    props.rendererConfig.sampleCount = 4;
    return new GEditor::GleamEditor(props);
}
