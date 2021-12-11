#pragma once

namespace Gleam {

class Window;
struct Version;

class GraphicsContext
{
public:

	GraphicsContext(const Window& window, const TString& appName, const Version& appVersion);
	~GraphicsContext();

};

} // namespace Gleam