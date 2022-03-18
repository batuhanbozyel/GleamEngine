#include "gpch.h"
#include "SceneLayer.h"

using namespace Gleam;

void SceneLayer::OnAttach()
{
	mRenderer = CreateScope<TileForwardRenderer>();
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

