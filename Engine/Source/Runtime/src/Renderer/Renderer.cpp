#include "gpch.h"
#include "Renderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"
#include "PipelineCache.h"

using namespace Gleam;

void Renderer::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
    RendererContext::Init(appName, appVersion, props);
	PipelineCache::Init();
    ShaderLibrary::Init();
}

void Renderer::Destroy()
{
	ShaderLibrary::Destroy();
	PipelineCache::Destroy();
    RendererContext::Destroy();
}
