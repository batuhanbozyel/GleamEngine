#pragma once
#include "Core/Window.h"

namespace Gleam {

class GraphicsContext
{
public:

	GraphicsContext(const TString& appName, const Window& window);
	~GraphicsContext();

};

} // namespace Gleam