#include "gpch.h"
#include "Renderer.h"
#include "RendererContext.h"
#include "ShaderLibrary.h"

using namespace Gleam;

static Scope<RendererContext> mContext = nullptr;

void Renderer::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
    mContext = CreateScope<RendererContext>(appName, appVersion, props);
}

void Renderer::Destroy()
{
	ShaderLibrary::ClearCache();
    mContext.reset();
}

void* Renderer::GetDevice()
{
	return mContext->GetDevice();
}

// Temporary
void Renderer::RenderFrame()
{
    mContext->BeginFrame();

	mContext->ClearScreen({ 1.0f, 0.8f, 0.4f, 1.0f });

    mContext->EndFrame();
}
