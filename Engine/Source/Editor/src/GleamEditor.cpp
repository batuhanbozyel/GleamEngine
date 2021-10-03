#include "Gleam.h"

namespace GEditor {

class GleamEditor : public Gleam::Application
{
public:

	GleamEditor()
		: Gleam::Application({"Gleam Editor", 1280, 720, Gleam::WindowFlag::MaximizedWindow})
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
	return new GEditor::GleamEditor;
}
