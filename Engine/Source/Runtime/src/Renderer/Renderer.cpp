#include "gpch.h"
#include "Renderer.h"
#include "Context.h"

using namespace Gleam;

Scope<Context> Renderer::mContext = nullptr;

void Renderer::Init(const TString& appName, const Version& appVersion, const RendererProperties& props)
{
	mContext = CreateScope<Context>(appName, appVersion, props);
}

void Renderer::Destroy()
{
	mContext.reset();
}

// Temporary
void Renderer::RenderFrame()
{
	mContext->BeginFrame();

	mContext->ClearScreen({ 1.0f, 0.8f, 0.4f, 1.0f });

	mContext->EndFrame();
}