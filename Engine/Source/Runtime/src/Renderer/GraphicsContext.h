#pragma once

struct SDL_Window;

namespace Gleam {

struct Version;

class GraphicsContext
{
public:

	GraphicsContext(SDL_Window* window, const TString& appName, const Version& appVersion);
	~GraphicsContext();

};

} // namespace Gleam