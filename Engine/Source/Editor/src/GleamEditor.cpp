// EntryPoint
#include "Core/EntryPoint.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::ApplicationProperties& props)
		: Gleam::Application(props), mSceneLayer(Gleam::CreateRef<Gleam::SceneLayer>())
	{
		PushLayer(mSceneLayer);
	}

	~GleamEditor()
	{
		RemoveLayer(mSceneLayer);
	}

private:

	Gleam::Ref<Gleam::SceneLayer> mSceneLayer;

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
