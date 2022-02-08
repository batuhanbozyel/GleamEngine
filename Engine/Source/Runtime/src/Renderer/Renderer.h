#pragma once

namespace Gleam {

struct Version;
struct RendererProperties;

class Renderer
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	static void BeginFrame();

	static void EndFrame();

	static void ClearScreen(const Color& color);

	/**
	* Renderer specific implementations
	*/
	virtual ~Renderer() = default;

	virtual void Initialize() = 0;

	virtual void Render() = 0;

	virtual void Shutdown() = 0;

};

} // namespace Gleam
