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

};

} // namespace Gleam
