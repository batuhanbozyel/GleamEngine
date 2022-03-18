#include "gpch.h"
#include "DebugLayer.h"

using namespace Gleam;

void DebugLayer::OnAttach()
{
	mRenderer = CreateScope<DebugRenderer>();
}

void DebugLayer::OnUpdate()
{

}

void DebugLayer::OnRender()
{
	mRenderer->Render();
}

void DebugLayer::OnDetach()
{
	mRenderer.reset();
}

