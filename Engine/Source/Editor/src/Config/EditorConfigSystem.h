#pragma once
#include "Core/ConfigSystem.h"

namespace GEditor {

class EditorConfigSystem final : public Gleam::ConfigSystem
{
public:

	EditorConfigSystem()
		: Gleam::ConfigSystem("Editor.config")
	{

	}

};

} // namespace GEditor
