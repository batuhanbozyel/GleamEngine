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

Vector2 Renderer::GetDrawableSize()
{
	return
	{
		static_cast<float>(RendererContext::GetProperties().width),
		static_cast<float>(RendererContext::GetProperties().height)
	};
}
