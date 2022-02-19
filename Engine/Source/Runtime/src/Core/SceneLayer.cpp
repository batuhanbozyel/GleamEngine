#include "gpch.h"
#include "SceneLayer.h"

using namespace Gleam;

void SceneLayer::OnAttach()
{
	mRenderer = CreateScope<SceneRenderer>();
}

void SceneLayer::OnUpdate()
{

}

void SceneLayer::OnRender()
{
	mRenderer->Render();
}

void SceneLayer::OnDetach()
{
	mRenderer.reset();
}

