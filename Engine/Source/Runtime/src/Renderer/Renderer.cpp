#include "gpch.h"
#include "Renderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"

using namespace Gleam;

void Renderer::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
    RendererContext::Init(appName, appVersion, props);
    ShaderLibrary::Init();
}

void Renderer::Destroy()
{
	ShaderLibrary::Destroy();
    RendererContext::Destroy();
}
