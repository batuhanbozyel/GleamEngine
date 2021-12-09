// EntryPoint
#include "Core/EntryPoint.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor(const Gleam::WindowProperties& props)
		: Gleam::Application(props)
	{

	}

	~GleamEditor()
	{

	}

private:

};

}

Gleam::Application* Gleam::CreateApplication()
{
	Gleam::WindowProperties props;
	props.title = "Gleam Editor";
	props.windowFlag = Gleam::WindowFlag::MaximizedWindow;
	return new GEditor::GleamEditor(props);
}
