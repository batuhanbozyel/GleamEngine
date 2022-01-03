#pragma once

namespace Gleam {

class Context;
struct Version;
struct RendererProperties;

class Renderer
{
public:

	static void Init(const TString& appName, const Version& appVersion, const RendererProperties& props);

	static void Destroy();

	// Temporary
	static void RenderFrame();

private:

	static Scope<Context> mContext;

};

} // namespace Gleam