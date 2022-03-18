#pragma once
#include "Layer.h"
#include "Renderer/DebugRenderer.h"

namespace Gleam {

class DebugLayer : public Layer
{
public:

	virtual void OnAttach() override;

	virtual void OnUpdate() override;

	virtual void OnRender() override;

	virtual void OnDetach() override;

private:

	Scope<DebugRenderer> mRenderer;

};

} // namespace Gleam