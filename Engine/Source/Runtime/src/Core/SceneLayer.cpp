#include "gpch.h"
#include "SceneLayer.h"

using namespace Gleam;

void SceneLayer::OnAttach()
{
	mRenderer = CreateScope<ForwardRenderer>();
}

void SceneLayer::OnUpdate()
{

}

void SceneLayer::OnFixedUpdate()
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

