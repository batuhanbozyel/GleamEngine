#define GDEBUG
// EntryPoint
#include "Core/EntryPoint.h"
#include "SceneView.h"

namespace GEditor {
    
class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::ApplicationProperties& props)
		: Gleam::Application(props)
	{
        PushView<Gleam::SceneView>();
	}

	~GleamEditor()
	{
        RemoveView<Gleam::SceneView>();
	}

private:

};

} // namespace GEditor

Gleam::Application* Gleam::CreateApplication()
{
	Gleam::ApplicationProperties props;
	props.appVersion = Gleam::Version(1, 0, 0);
	props.windowProps.title = "Gleam Editor";
	props.windowProps.windowFlag = Gleam::WindowFlag::MaximizedWindow;
	return new GEditor::GleamEditor(props);
}
