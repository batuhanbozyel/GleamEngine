#pragma once
#include "Layer.h"
#include "Renderer/TileForwardRenderer.h"

namespace Gleam {

class SceneLayer : public Layer
{
public:

	virtual void OnAttach() override;

	virtual void OnUpdate() override;

	virtual void OnRender() override;

	virtual void OnDetach() override;

private:

	Scope<TileForwardRenderer> mRenderer;

};

} // namespace Gleam