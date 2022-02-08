#include "gpch.h"
#include "Layer.h"
#include "Renderer/Renderer.h"

using namespace Gleam;

/************************************************************************/
/*  Layer                                                               */
/************************************************************************/
void Layer::SetRenderer(Renderer* renderer)
{
	mRenderer = Scope<Renderer>(renderer);
}

void Layer::OnAttachSuper()
{
	if (mRenderer)
	{
		mRenderer->Initialize();
	}
	OnAttach();
}

void Layer::OnDetachSuper()
{
	if (mRenderer)
	{
		mRenderer->Shutdown();
	}
	OnDetach();
}

void Layer::OnRenderSuper()
{
	if (mRenderer)
	{
		mRenderer->Render();
	}
	OnRender();
}
