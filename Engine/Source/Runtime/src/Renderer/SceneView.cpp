#include "gpch.h"
#include "SceneView.h"
#include "RendererContext.h"

using namespace Gleam;

void SceneView::OnUpdate()
{
	mWorld.Update();
}

void SceneView::OnFixedUpdate()
{
	mWorld.FixedUpdate();
}

void SceneView::OnRender(RendererContext& ctx, RenderTargetIdentifier target)
{
	mWorld.Render(ctx);
}